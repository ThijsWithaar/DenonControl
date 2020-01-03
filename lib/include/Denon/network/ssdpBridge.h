#pragma once

#include <Denon/network/ssdp.h>
#include <Denon/network/ssdpIpc.h>


namespace Ssdp {


/// Bridge SSDP accross network interfaces and minissdp's IPC
class Bridge
{
public:
	struct Settings
	{
		Interface server, client;
		MiniSsdp::Server::Settings ipc;
	};

	Bridge(boost::asio::io_context& io_context, Settings settings);

private:
	void OnReceiveFromClient(boost::asio::ip::address_v4 sender, const std::string_view& data);
	void OnReceiveFromServer(boost::asio::ip::address_v4 sender, const std::string_view& data);

	ServiceCache m_cache;
	Connection m_Client, m_Server;
	MiniSsdp::Server m_Ipc;
};


} // namespace Ssdp
