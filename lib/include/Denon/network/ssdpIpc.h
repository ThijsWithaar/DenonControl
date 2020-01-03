#pragma once

#include <Denon/network/ssdp.h>


namespace MiniSsdp {


/** Implementation of the IPC mechanism from minissdpd
 See: https://miniupnp.tuxfamily.org/minissdpd.html
	  http://manpages.org/minissdpd
*/
class Server
{
public:
	struct Settings
	{
		Settings();

		std::string socketName;
	};

	/// https://github.com/miniupnp/miniupnp/blob/master/minissdpd/minissdpd.c
	enum class RequestType {
		GetVersion=0, Type=1, Usn=2, All=3, Submit=4, Notify=5
	};

	Server(
		boost::asio::io_context& context,
		Ssdp::ServiceCache& cache,
		Settings s=Settings());

private:
	void ScheduleAccept();
	void ScheduleRx();

	std::string_view ReadString();
	void GetIf(std::function<bool(const Ssdp::Notify&)> condition);

	void GetVersion();
	void GetType();
	void GetUSN();
	void GetAll();

	boost::asio::local::stream_protocol::acceptor m_acceptor;
	boost::asio::local::stream_protocol::socket m_socket;
	boost::asio::streambuf m_rxBuf;
	std::vector<char> m_txBuf;
	Ssdp::ServiceCache& m_cache;
};

/// IPC protocol of minissdp
class Client
{
public:
	using Settings = Server::Settings;

	Client(boost::asio::io_context& ioc, Settings s=Settings());

	std::string GetVersion();
	std::vector<Ssdp::Response> SearchUsn(std::string usn);
	std::vector<Ssdp::Response> SearchType(std::string usn);
	std::vector<Ssdp::Response> RequestAll();

	void Submit(Ssdp::Notify nt);

private:
	std::vector<std::string_view> Dispatch(Server::RequestType rqType);
	static std::vector<Ssdp::Response> SplitResponses(std::vector<std::string_view> strings);

	boost::asio::local::stream_protocol::socket m_socket;
	Settings m_settings;
	boost::asio::streambuf m_rxBuf;
	std::vector<char> m_txBuf;
};

} // namespace Ssdp
