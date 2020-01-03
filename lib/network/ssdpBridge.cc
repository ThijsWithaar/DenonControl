#include <Denon/network/ssdpBridge.h>

#define DEBUG_LVL 1
#ifdef DEBUG_LVL
	#include <iostream>
#endif


namespace Ssdp {


Bridge::Bridge(boost::asio::io_context& io_context, Settings settings):
	m_Client(io_context, boost::asio::ip::make_address_v4(settings.client.ip), [this](auto sender, auto msg){ this->OnReceiveFromClient(sender, msg); }),
	m_Server(io_context, boost::asio::ip::make_address_v4(settings.server.ip), [this](auto sender, auto msg){ this->OnReceiveFromServer(sender, msg); }),
	m_Ipc(io_context, m_cache, settings.ipc)
{
}


void Bridge::OnReceiveFromClient(boost::asio::ip::address_v4 sender, const std::string_view& data)
{
	auto sn = Parse(data);
	if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
	{
		ServiceCache::Key key{pNotify->nt, pNotify->location};
		#if DEBUG_LVL > 1
			if(m_cache.services.count(key) == 0)
				std::cout << "Client(" << sender.to_string() <<") Notify:\t" << pNotify->nt << std::endl;
		#endif
		m_cache.services[key] = *pNotify;
	}
	else if(auto pSearch = std::get_if<Ssdp::Search>(&sn))
	{
		#if DEBUG_LVL > 1
			std::cout << "Client(" << sender.to_string() <<") Search:\t" << pSearch->searchType << std::endl;
		#endif
		for(auto& it: m_cache.services)
		{
			if(it.first.urn == pSearch->searchType || pSearch->searchType == "ssdp:all")
			{
				#if DEBUG_LVL > 1
					std::cout << "Bridge->Client(" << sender.to_string() << "):\t" << it.second.nt << std::endl;
				#endif
				std::string msg = Response(it.second);
				m_Client.send(sender, msg);
			}
		}
	}
    else
	{
		#if DEBUG_LVL > 0
			std::cout << "Client->Server: Unknown\n" << data << std::endl;
		#endif
	}

	m_Server.broadcast(data);
}


void Bridge::OnReceiveFromServer(boost::asio::ip::address_v4 sender, const std::string_view& data)
{
	bool m_cacheNotify = true;
	auto sn = Parse(data);
	if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
	{
		if(m_cacheNotify)
		{
			ServiceCache::Key key{pNotify->nt, pNotify->location};
			#if DEBUG_LVL > 0
				if(m_cache.services.count(key) == 0)
					std::cout << "server(" << sender.to_string() <<") new notify: " << pNotify->nt << std::endl;
			#endif
			m_cache.services[key] = *pNotify;
		}

		//std::cout << "Server: Notify -> client\n";
		//m_Client.send(data);
	}
	else if(auto pResponse = std::get_if<Ssdp::Response>(&sn))
	{
		ServiceCache::Key key{pResponse->nt, pResponse->location};
		#if DEBUG_LVL > 0
			if(m_cache.services.count(key) == 0)
				std::cout << "New service from server(" << sender.to_string() <<") by response:\t" << pResponse->nt << " at " << pResponse->location << std::endl;
		#endif
		m_cache.services[key] = Notify(*pResponse);

		#if DEBUG_LVL > 1
			std::cout << "Server: Response as Notify -> client\n";
		#endif
		std::string msg = Notify(*pResponse);
		m_Client.broadcast(msg);

		// TODO: Match to outstanding searches, and reply to specific IP
		//m_Client.send(,msg);
	}
	else
	{
		#if DEBUG_LVL > 1
			std::cout << "Server(" << sender.to_string() <<"): Other (search?) -> Client\n" << data << std::endl;
		#endif
		m_Client.broadcast(data);
	}
}


} // namespace Ssdp
