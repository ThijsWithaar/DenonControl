#include <Denon/network/http.h>

#include <charconv>
#include <iostream>
#include <fstream>

#include <boost/asio/connect.hpp>
#include <boost/asio/read_until.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <Denon/string.h>


namespace Denon {
namespace Http {


template<typename Enum>
std::pair<std::string, Enum> enumStrPair(Enum e)
{
	std::stringstream ss;
	ss << e;
	return {ss.str(), e};
}


std::ostream& operator<<(std::ostream& os, Method m)
{
	switch(m)
	{
		case Method::Get: os << "GET"; break;
		case Method::Post: os << "POST"; break;
		case Method::Search: os << "SEARCH"; break;
		case Method::Notify: os << "NOTIFY"; break;
		case Method::Subscribe: os << "SUBSCRIBE"; break;
		case Method::MSearch: os << "M-SEARCH"; break;
	}
	return os;
}


const std::map<std::string, Method> g_MethodLut = {
	enumStrPair(Method::Get),
	enumStrPair(Method::Post),
	enumStrPair(Method::Search),
	enumStrPair(Method::Notify),
	enumStrPair(Method::Subscribe),
	enumStrPair(Method::MSearch),
};


Request::operator std::string() const
{
	std::stringstream r;
	r << method << " " << path << " HTTP/1.1\r\n";
	for(auto& field: fields)
		r << field.first << ": " << field.second << "\r\n";
	r << "CONTENT-LENGTH: " << body.size() << "\r\n";
	r << "\r\n";
	if(!body.empty())
		r << body;
	return r.str();
}


Response::operator std::string() const
{
	std::string r;
	r = "HTTP/1.1 " + std::to_string(status) + " OK\r\n";
	r += "CONTENT-LENGTH: " + std::to_string(body.size()) + "\r\n";
	for(auto& f: fields)
	{
		r += f.first + ": " + f.second + "\r\n";
	}
	r += "\r\n";
	r += body;
	return r;
}


void ParseHeader(Http::Response& dst, std::string_view data)
{
	//std::cout << "ParseHttpHeader\n" << data << "\n";
	bool firstLine = true;
	splitlines(data, [&](auto line)
	{
		if(firstLine)
		{
			auto els = split(line);
			if(dst.method == Method::MSearch)
			{
				std::string mstr{els[0]};
				if(g_MethodLut.count(mstr) == 0)
					throw std::runtime_error("ParseHeader: method not found " + mstr);
				dst.method = g_MethodLut.at(mstr);
				dst.path = els[1];
			}
			else
			{
				std::from_chars(els[1].data(), els[1].data() + els[1].size(), dst.status);
			}
			firstLine = false;
			return;
		}
		auto [key, value] = splitKeyVal(line);
		dst.fields[std::string(key)] = value;
		//std::cout << "ParseHttpHeader: " << key << " : " << value << "\n";
	});
}


//-- Parser --


Parser::Parser(Method method):
	m_idx(0),
	m_method(method)
{
}


std::pair<char*, size_t> Parser::currentBuffer()
{
	//std::cout << "currentBuffer: idx = " << m_idx <<"\n";
	return {m_buf.data() + m_idx, m_buf.size() - m_idx};
}


const Http::Response* Parser::markRead(size_t& nRead)
{
	//std::cout << "markRead: idx = " << m_idx << ", n = " << n <<"\n";
	m_idx += nRead;
	nRead = 0;
	auto bufEnd = m_buf.data() + m_idx;

	constexpr std::array<char, 4> eoh = {'\r','\n','\r','\n'};
	auto hEnd = std::search(m_buf.data(), bufEnd, eoh.begin(), eoh.end());
	if(hEnd == bufEnd)
		return nullptr;

	m_packet.method = m_method;
	ParseHeader(m_packet, std::string_view(m_buf.data(), hEnd-m_buf.data()));
	auto clens = m_packet.fields["CONTENT-LENGTH"];
	if(clens.empty())
		return nullptr;		// This parser requires content-length

	auto dBegin = hEnd + 4;
	size_t clen = std::stoi(clens);
	size_t dlen = std::distance(dBegin, bufEnd);
	if(dlen < clen)
		return nullptr;		// Data not yet complete

	// Need to copy, because of memmove below
	m_packet.body = std::string_view(dBegin, clen);
	//std::cout << "*HttpParser body*\n'" << m_packet.body << "'\n";

	auto remBegin = dBegin + clen;
	if(dlen > clen)
	{
		//std::cout << "*HttpParser: keeping " << std::distance(remBegin, bufEnd) << " bytes\n";
		std::memmove(m_buf.data(), remBegin, std::distance(remBegin, bufEnd));
	}
	m_idx = dlen - clen;

	return &m_packet;
}


//-- BlockingConnection --


#ifdef HAS_BOOST_BEAST
BeastConnection::BeastConnection(boost::asio::io_context& ioc, std::string host, int port):
	m_socket(ioc),
	m_host(host)
{
	tcp::resolver resolver(ioc);
	m_resolve = resolver.resolve(host, std::to_string(port));
}


void BeastConnection::setCaptureFilename(std::string fname)
{
	m_capture.open(fname, std::ios::binary);
}


const Response& BeastConnection::Http(const Request& req)
{
	namespace http = boost::beast::http;		// from <boost/beast/http.hpp>
	namespace asio = boost::asio;

	http::verb method;
	switch(req.method)
	{
		case Method::Get: method = http::verb::get; break;
		case Method::Post: method = http::verb::post; break;
		case Method::Search: method = http::verb::search; break;
		case Method::Notify: method = http::verb::notify; break;
		case Method::Subscribe: method = http::verb::subscribe; break;
		case Method::MSearch: method = http::verb::msearch; break;
	}
	http::request<http::string_body> hreq{method, req.path, 11};
	hreq.set(http::field::host, m_host);
	hreq.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	for(auto field: req.fields)
		hreq.set(field.first, field.second);
	if(!req.body.empty())
		hreq.body() = req.body;
	hreq.prepare_payload();

	boost::asio::connect(m_socket, m_resolve.begin(), m_resolve.end());
	http::write(m_socket, hreq);

	http::response<http::dynamic_body> res;
	auto nRead = http::read(m_socket, m_buffer, res);

	m_socket.close();

	if(m_capture.is_open())
	{
		// Re-construct the header, don't know how else to get it
		m_capture << "HTTP/1.1 " << res.result_int() << " " << res.result() << "\r\n";
		for(auto& field: res)
			m_capture << field.name_string() << ": " << field.value() << "\r\n";
		m_capture << "\r\n";
		m_capture << boost::beast::buffers_to_string(res.body().data());
		m_capture.flush();
	}

	auto bdata = res.body().data();
	m_response.status = res.result_int();
	m_response.body = {asio::buffers_begin(bdata), asio::buffers_end(bdata)};

	return m_response;
}
#endif


} // namespace Http
} // namespace Denon
