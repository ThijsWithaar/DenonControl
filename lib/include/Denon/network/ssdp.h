#pragma once

#include <array>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>

#include <boost/asio.hpp>
#include <boost/bind.hpp>


namespace Ssdp {

constexpr int port = 60006;
constexpr char path[] = "/upnp/desc/aios_device/aios_device.xml";

/// Expand fname into an absolute path.
/// $HOME/fname if it exists, otherwise /etc/fname
std::string GetConfigurationPath(std::string fname, bool system);

/// Description of a network interface
struct Interface
{
	std::string name, ip, netmask;
};

/// SSDP Search request
struct Search
{
	std::string searchType;		/// urn to search for
	std::map<std::string, std::string> custom;

	operator std::string();
	bool operator==(const Search& o) const;
};

/// SSDP Search response
struct Response
{
	std::string location;	/// url of where the service is
	std::string nt;			/// urn being notified
	std::map<std::string, std::string> custom;

	operator std::string();
	bool operator==(const Response& o) const;
};

/// SSDP Notify (broadcast, not requested)
struct Notify: public Response
{
	Notify() = default;
	explicit Notify(const Response& r);
	operator std::string();
	bool operator==(const Notify& o) const;
};

std::variant<Search, Response, Notify> Parse(std::string_view message);

/// Returns a list of network interfaces
std::vector<Interface> GetNetworkInterfaces();

class ServiceCache
{
public:
	struct Key
	{
		std::string urn, location;

		bool operator<(const Key& o) const;
	};

	std::map<Key, Notify> services;
};

/// SSDP connection over UDP, linked to a single interface
class Connection
{
public:
	using rx_callback_t = std::function<void(boost::asio::ip::address_v4, std::string_view)>;

	/// Shared between listen- and broadcast-connection
	class Base
	{
	public:
		Base(boost::asio::io_context& context, const boost::asio::ip::address_v4& itf, rx_callback_t onRx);

	protected:
		void send(boost::asio::ip::udp::endpoint dst, const std::string_view& data);

		void ScheduleRx();
		boost::asio::ip::udp::socket m_socket;

	private:
		boost::asio::ip::address_v4 m_itf;
		rx_callback_t m_cb;
		boost::asio::ip::udp::endpoint m_sender_endpoint;
		std::array<char, 1<<10> m_rxBuf, m_txBuf;
	};

	/// Listen to UDP on a interface:port, to receive broadcast replies
	class Listen: public Base
	{
	public:
		Listen(boost::asio::io_context& context, const boost::asio::ip::address_v4& itf, rx_callback_t onRx);

		void send(const boost::asio::ip::address_v4& dst, const std::string_view& data);
	};

	/// Multicast connection, to send/receive multicast messages
	class MultiCast: public Base
	{
	public:
		MultiCast(boost::asio::io_context& context, const boost::asio::ip::address_v4& itf, rx_callback_t onRx);

		void broadcast(const std::string_view& data);
	};

	Connection(
		boost::asio::io_context& io_context,
		const boost::asio::ip::address_v4& interfaceAddress,	///< Address of the interface to listen to
		rx_callback_t onReceive
	);

	/// Broadcast on the multicast address:port
	void broadcast(const std::string_view& data);

	/// Send to a specific client
	void send(const boost::asio::ip::address_v4& dst, const std::string_view& data);

private:
	MultiCast m_multicast;
	Listen m_listen;
};

/// SSDP Client, can search, with caching of past notifies
class Client
{
public:
	using callback_t = std::function<void(const boost::asio::ip::address_v4, Notify)>;

	Client(
		boost::asio::io_context& io_context,
		const boost::asio::ip::address_v4& interfaceAddress,	///< Address of the interface to listen to
		callback_t notify);

	/// Start a search
	void Search(std::string urn);

private:
	void OnReceive(boost::asio::ip::address_v4 sender, std::string_view msg);

	ServiceCache m_cache;
	Connection m_connection;
	callback_t m_notify;
};

} // namespace Ssdp
