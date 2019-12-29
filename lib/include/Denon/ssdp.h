#pragma once

#include <array>
#include <functional>
#include <string_view>
#include <variant>

#include <boost/asio.hpp>
#include <boost/bind.hpp>


namespace Ssdp {

/// Expand fname into an absolute path.
/// $HOME/fname if it exists, otherwise /etc/fname
std::string GetConfigurationPath(std::string fname, bool create);

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
};

/// SSDP Search response
struct Notify
{
	std::string location;	/// url of where the service is
	std::string nt;			/// urn being notified
	std::map<std::string, std::string> custom;
};

std::variant<Search, Notify> Parse(std::string_view message);

/// Returns a list of network interfaces
std::vector<Interface> GetNetworkInterfaces();

/// SSDP connection over UDP, linked to a single interface
class Connection
{
public:
	using rx_callback_t = std::function<void(std::string_view)>;

	Connection(
		boost::asio::io_context& io_context,
		const boost::asio::ip::address_v4& interfaceAddress,	///< Address of the interface to listen to
		rx_callback_t onReceive
	);

	/// Send data over this interface
	void send(const std::string_view& data);

private:
	void schedule_receive();
	void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
	void handle_send_to(const boost::system::error_code& error);

	boost::asio::ip::udp::socket m_socket;
	boost::asio::ip::udp::endpoint m_listen_endpoint;	///< Our own endpoint
	boost::asio::ip::udp::endpoint m_sender_endpoint;	///< endpoint of other party
	boost::asio::ip::address_v4 m_itf_address;
	std::array<char, 1<<10> m_data;
	std::vector<char> m_sendData;
	rx_callback_t m_Callback;
};

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

/// SSDP Client, can search, with caching of past notifies
class Client
{
public:
	using callback_t = std::function<void(Notify)>;

	Client(
		boost::asio::io_context& io_context,
		const boost::asio::ip::address_v4& interfaceAddress,	///< Address of the interface to listen to
		callback_t notify);

	/// Start a search
	void Search(std::string urn);

private:
	void OnReceive(std::string_view msg);

	ServiceCache m_cache;
	Connection m_connection;
	callback_t m_notify;
};

/// Bridge SSDP accross two interfaces
class Bridge
{
public:
	struct Settings
	{
		Interface server, client;
	};

	Bridge(boost::asio::io_context& io_context, Settings settings);

private:
	void OnReceiveFromClient(const std::string_view& data);
	void OnReceiveFromServer(const std::string_view& data);

	Connection m_Client, m_Server;
	ServiceCache m_cache;
};


} // namespace Ssdp
