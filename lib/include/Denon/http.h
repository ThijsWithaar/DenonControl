#pragma once

#include <map>

#define HAS_BOOST_BEAST
#ifdef HAS_BOOST_BEAST
	#include <boost/beast/core.hpp>
	#include <boost/asio/ip/tcp.hpp>
#endif

#include <Denon/denon.h>
#include <Denon/serial.h>


namespace Denon {
namespace Http {


enum class Method
{
	Get, Post, Search, Notify, Subscribe, MSearch
};

struct Request
{
	Method method;
	std::string path;	///< relative path
	std::map<std::string, std::string> fields;
	std::string body;

	operator std::string() const;
};

struct Response: public Request
{
	int status;
	operator std::string() const;
};

/**
	Http parser, assuming messages come in pieces.
	Similar to boost::beast. Operates on buffers, to allow integration into Qt

	currentBuffer() and markRead() put data into the buffer
*/
class Parser
{
public:
	Parser(Method m);

	// Current buffer to write into
	std::pair<char*, size_t> currentBuffer();

	// Mark n bytes as read, returns true if packet is complete.
	// should be called until it returns nullptr to parse all packets
	const Response* markRead(size_t& nRead);

private:
	std::array<char, 1<<14> m_buf;
	size_t m_idx;
	Method m_method;
	Response m_packet;
};

/// Interface for an HTTP connection, to allow both boost::asio and Qt
class BlockingConnection
{
public:
	virtual ~BlockingConnection() = default;

	virtual const Response& Http(const Request& req) = 0;
};

#ifdef HAS_BOOST_BEAST
/// Http connection using boost::beast
class BeastConnection: public BlockingConnection
{
public:
	BeastConnection(boost::asio::io_context& ioc, std::string host, int port);

	const Response& Http(const Request& req) override;

private:
	using tcp = boost::asio::ip::tcp;			// from <boost/asio/ip/tcp.hpp>

	std::string m_host;
	boost::beast::flat_buffer m_buffer;
	tcp::socket m_socket;

	Response m_response;
};
#endif


} // namespace Http
} // namespace Denon
