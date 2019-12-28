#include <Denon/upnpEvent.h>

#include <iostream>

#include <Denon/string.h>


namespace Upnp {


void ParseHttpHeader(HttpParser::Header& dst, std::string_view data)
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
	});
}


//-- Response --


Response::Response(int statusCode):
	m_statusCode(statusCode)
{
}


Response::operator std::string()
{
	std::string r;
	r = "HTTP/1.1 " + std::to_string(m_statusCode) + " OK\r\n";
	r += "CONTENT-LENGTH: " + std::to_string(body.size()) + "\r\n";
	for(auto& f: fields)
	{
		r += f.first + ": " + f.second + "\r\n";
	}
	r += "\r\n";
	r += body;
	return r;
}


//-- HttpParser --


HttpParser::HttpParser():
	m_idx(0)
{
}


std::pair<char*, size_t> HttpParser::currentBuffer()
{
	return {m_buf.data() + m_idx, m_buf.size() - m_idx};
}


const HttpParser::Packet* HttpParser::markRead(size_t n)
{
	m_idx += n;
	auto bufEnd = m_buf.data() + m_idx;

	constexpr std::array<char, 4> eoh = {'\r','\n','\r','\n'};
	auto hEnd = std::search(m_buf.data(), bufEnd, eoh.begin(), eoh.end());
	if(hEnd == bufEnd)
		return nullptr;

	ParseHttpHeader(m_packet.header, std::string_view(m_buf.data(), hEnd-m_buf.data()));
	auto clens = m_packet.header.options["CONTENT-LENGTH"];
	if(clens.empty())
		return nullptr;		// This parser requires content-length

	auto dBegin = hEnd + 4;
	auto dEnd = bufEnd;

	size_t clen = std::stod(clens);
	size_t dlen = std::distance(dBegin, dEnd);
	if(dlen < clen)
		return nullptr;		// Data not yet complete

	m_packet.body = std::string_view(dBegin, dlen);
	m_idx = 0;
	return &m_packet;
}


Subscribe::operator std::string()
{
	std::string r;
	r += "SUBSCRIBE " + url + " HTTP/1.1\r\n";
	r += "HOST: " + host + "\r\n";
	r += "CALLBACK: " + callback + "\r\n";
	r += "NT: " + type + "\r\n";
	r += "TIMEOUT: " + timeOut + "\r\n";
	r += "\r\n";

	return r;
}


} // namespace Upnp

