#include <Denon/http.h>

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



void ParseHeader(Http::Parser::Header& dst, std::string_view data)
{
	//std::cout << "ParseHttpHeader\n" << data << "\n";
	dst.method.clear();
	splitlines(data, [&](auto line)
	{
		if(dst.method.empty())
		{
			auto els = split(line);
			dst.method = els[0];
			dst.url = els[1];
			return;
		}
		auto [key, value] = splitKeyVal(line);
		dst.options[std::string(key)] = value;
		//std::cout << "ParseHttpHeader: " << key << " : " << value << "\n";
	});
}

//-- Parser --


Parser::Parser():
	m_idx(0)
{
}


std::pair<char*, size_t> Parser::currentBuffer()
{
	//std::cout << "currentBuffer: idx = " << m_idx <<"\n";
	return {m_buf.data() + m_idx, m_buf.size() - m_idx};
}


const Parser::Packet* Parser::markRead(size_t n)
{
	//std::cout << "markRead: idx = " << m_idx << ", n = " << n <<"\n";
	m_idx += n;
	auto bufEnd = m_buf.data() + m_idx;

	constexpr std::array<char, 4> eoh = {'\r','\n','\r','\n'};
	auto hEnd = std::search(m_buf.data(), bufEnd, eoh.begin(), eoh.end());
	if(hEnd == bufEnd)
		return nullptr;

	ParseHeader(m_packet.header, std::string_view(m_buf.data(), hEnd-m_buf.data()));
	auto clens = m_packet.header.options["CONTENT-LENGTH"];
	if(clens.empty())
		return nullptr;		// This parser requires content-length

	auto dBegin = hEnd + 4;
	size_t clen = std::stod(clens);
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

std::ostream& operator<<(std::ostream& os, BlockingConnection::Method m)
{
	using Method = BlockingConnection::Method;
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


BlockingConnection::Request::operator std::string()
{
	std::stringstream r;
	r << method << " " << path << " HTTP/1.1\r\n";
	for(auto& field: fields)
		r << field.first << ": " << field.second << "\r\n";
	r << "\r\n";
	return r.str();
}


BlockingConnection::Response::operator std::string()
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


#ifdef HAS_BOOST_BEAST
BeastConnection::BeastConnection(boost::asio::io_context& ioc, std::string host, int port):
	m_socket(ioc),
	m_host(host)
{
	tcp::resolver resolver(ioc);
	auto const results = resolver.resolve(host, std::to_string(port));
	boost::asio::connect(m_socket, results.begin(), results.end());
}


const BlockingConnection::Response& BeastConnection::Http(const Request& req)
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
	if(!req.body.empty())
		hreq.body() = req.body;
	hreq.prepare_payload();

	http::write(m_socket, hreq);

	http::response<http::dynamic_body> res;
	http::read(m_socket, m_buffer, res);

	auto bdata = res.body().data();
	m_response.status = res.result_int();
	m_response.body = {asio::buffers_begin(bdata), asio::buffers_end(bdata)};

	return m_response;
}
#endif


} // namespace Http
} // namespace Denon
