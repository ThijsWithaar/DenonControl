#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>


namespace Upnp {


/// Similar to boost::beast. Operates on buffers, to allow
/// integration into Qt
class Response
{
public:
	Response(int statusCode);

	operator std::string();

	std::map<std::string, std::string> fields;
	std::string body;
private:
	int m_statusCode;
};

/*
	Http parser, assuming messages come in pieces.
		currentBuffer() and markRead() put data into the buffer
*/
class HttpParser
{
public:
	struct Header
	{
		std::string method, url;
		std::map<std::string, std::string> options;
	};

	struct Packet
	{
		Header header;
		std::string body;
	};

	HttpParser();

	// Current buffer to write into
	std::pair<char*, size_t> currentBuffer();

	// Mark n bytes as read, returns true if packet is complete.
	// should be called until it returns nullptr to parse all packets
	const Packet* markRead(size_t n);

private:
	std::array<char, 1<<14> m_buf;
	size_t m_idx;
	Packet m_packet;
};

/// Subscribe message
struct Subscribe
{
	std::string url;		///< upnp/event/renderer_dvc/ConnectionManager
	std::string host;		///< 192.168.4.7:60006
	std::string callback;	///< <http://192.168.3.11:49200/>
	std::string type;		///< upnp:event
	std::string timeOut;	///< Second-180

	operator std::string();
};


} // namespace Upnp
