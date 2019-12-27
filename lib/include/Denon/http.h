#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read_until.hpp>


#include <Denon/denon.h>
#include <Denon/serial.h>


namespace Denon {


namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;		// from <boost/beast/http.hpp>
namespace asio = boost::asio;		// from <boost/asio.hpp>
using tcp = asio::ip::tcp;			// from <boost/asio/ip/tcp.hpp>

// https://blue-pc.net/2013/12/28/denon-av-reciever-ueber-http-steuern/
//
// Connect to
//	http://192.168.4.7:8080/goform/Deviceinfo.xml

struct DeviceCapabilities
{
};

struct ZoneCapabilities
{
};

struct DeviceInfo
{
	int version;
	int commApiVersion;
	std::string catName;
	std::string modelName;
	std::string manualModelName;
	std::string mac;
	DeviceCapabilities capabilities;
	std::vector<ZoneCapabilities> zoneCapabilities;
};

struct ZoneStatus
{
	std::string power;
	std::string source;
	double volume;
	std::string mute;
};

struct DeviceStatus
{
	std::map<std::string, ZoneStatus> zones;
	std::string surround;
};

class HttpDevice:
	public CommandConnection
{
public:
	HttpDevice(std::string host, int port=8080);

	DeviceInfo GetDeviceInfo();

	DeviceStatus GetDeviceStatus();

	/// CommandConnection
	void Send(const std::string&) override;

private:
	/// HTTP get request, for relative path
	http::response<http::dynamic_body> Get(std::string path);
	http::response<http::dynamic_body> Post(std::string path, std::string body);

	// Application Command
	http::response<http::dynamic_body> AppCommand(std::vector<std::string> attributes);

	asio::io_context m_ioc;
	tcp::socket m_socket;
	std::string m_host;
	boost::beast::flat_buffer m_buffer;
};


} // namespace Denon
