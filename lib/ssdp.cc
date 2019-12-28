#include <Denon/ssdp.h>

#include <iostream>
#include <filesystem>

#ifdef __linux__
	#include <ifaddrs.h>
	#include <pwd.h>
#endif


#include <Denon/string.h>


namespace Ssdp {


constexpr short multicast_port = 1900;

const boost::asio::ip::address_v4 multicast_address = boost::asio::ip::make_address_v4("239.255.255.250");


bool sameSubnet(
		const boost::asio::ip::address_v4& a,
		const boost::asio::ip::address_v4& b,
		int mask
		)
{
	std::uint32_t bmask = (0xFFFFFFFF << (32-mask));
	std::uint32_t neta = a.to_ulong() & bmask;
	std::uint32_t netb = b.to_ulong() & bmask;
	return neta == netb;
}


#ifdef __linux__


std::string GetConfigurationPath(std::string fname, bool create)
{
	const char *homedir;
	if((homedir = getenv("HOME")) == nullptr)
		homedir = getpwuid(getuid())->pw_dir;

	std::string ret = std::string(homedir) + "/." + fname;
	if(std::filesystem::exists(ret) || create)
		return ret;
	return "/etc/" + fname;
}


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


std::vector<Interface> GetNetworkInterfaces()
{
	std::vector<Interface> ret;

	ifaddrs* ifaddr;
	if(getifaddrs(&ifaddr) == -1)
		return ret;

	for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if(ifa->ifa_addr == nullptr)
			continue;
		if(ifa->ifa_addr->sa_family != AF_INET)
			continue;
		char ipAddress[INET_ADDRSTRLEN], netMask[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, get_in_addr(ifa->ifa_addr), ipAddress, INET_ADDRSTRLEN);
		inet_ntop(AF_INET, get_in_addr(ifa->ifa_netmask), netMask, INET_ADDRSTRLEN);

		ret.push_back({ifa->ifa_name, ipAddress, netMask});
	}

	return ret;
}
#endif


//-- Parsing --


using QueryEntries = std::map<std::string_view, std::string&>;


void ParseSddp(
		std::string_view message,
		const QueryEntries& qe,
		std::map<std::string, std::string>& custom
		)
{
	std::string_view hdr;
	splitlines(message, [&](auto line)
	{
		if(hdr.empty())
		{
			hdr = line;
			return;
		}
		if(line.empty())
			return;
		auto [key, val] = splitKeyVal(line);
		auto id = qe.find(key);
		if(id != qe.end())
			id->second = val;
		else
			custom.insert({std::string(key), std::string(val)});
	});
};


Search ParseSearch(std::string_view message)
{
	Search s;
	QueryEntries qe = {
		{"ST", s.searchType},
	};

	ParseSddp(message, qe, s.custom);
	return s;
}


Search::operator std::string()
{
	std::string r;
	r += "M-SEARCH * HTTP/1.1\r\n";
	r += "HOST: 239.255.255.250:1900\r\n";
	r += "MAN: \"ssdp:discover\"\r\n";
	r += "MX: 10\r\n";
	r += "ST: " + searchType + "\r\n";
	r += "\r\n";
	return r;
}


Notify ParseNotify(std::string_view message)
{
	Notify n;
	QueryEntries qe = {
		{"LOCATION", n.location},
		{"NT", n.nt},
	};

	ParseSddp(message, qe, n.custom);
	return n;
}


std::variant<Search, Notify> Parse(std::string_view message)
{
	if(startswith(message, "M-SEARCH"))
		return ParseSearch(message);
	else
		return ParseNotify(message);
}


//-- Connection --


Connection::Connection(
			boost::asio::io_context& io_context,
			const boost::asio::ip::address_v4& listen_address,
			rx_callback_t onReceive
			):
		m_socket(io_context),
		m_Callback(onReceive),
		m_itf_address(listen_address),
		m_listen_endpoint(multicast_address, multicast_port)
{
	// Create the socket so that multiple may be bound to the same address.
	m_socket.open(m_listen_endpoint.protocol());
	m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	m_socket.bind(m_listen_endpoint);

	// Join the multicast group
	// Limiting to the listen_address works in windows, not in linux.
	// https://stackoverflow.com/questions/31101047/getting-the-destination-address-of-udp-packet
	m_socket.set_option(boost::asio::ip::multicast::join_group(multicast_address, listen_address));

	// Limit sending to only this interface
	boost::asio::ip::multicast::outbound_interface option( listen_address );
	m_socket.set_option(option);

	schedule_receive();

	std::cout << "SSDP Connection on interface: " << listen_address.to_string() << std::endl;
}


void Connection::send(const std::string_view& data)
{
	m_sendData.assign(data.begin(), data.end());

	m_socket.async_send_to(
		boost::asio::buffer(m_sendData),
		m_listen_endpoint,
		boost::bind(&Connection::handle_send_to, this, boost::asio::placeholders::error)
	);
}


void Connection::handle_send_to(const boost::system::error_code& error)
{
	m_sendData.clear();
}


void Connection::schedule_receive()
{
	m_socket.async_receive_from(
		boost::asio::buffer(m_data.data(), m_data.size()),
		m_sender_endpoint,
		boost::bind(&Connection::handle_receive_from, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
	);
}


void Connection::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
	if(error)
	{
		std::cout << "RX error\n";
		schedule_receive();
		return;
	}

	// Since linux does not do UDP interface filtering, do it by hand
	auto ipS = m_sender_endpoint.address().to_v4();
	auto ipItf = m_itf_address;
	if(sameSubnet(ipS, ipItf, 24) && (ipS != ipItf))
	{
		//std::cout << "RX from " << ipS << ", interface " << ipItf << "\n";
		std::string_view msg(m_data.data(), bytes_recvd);
		m_Callback(msg);
	}

	schedule_receive();
}


//-- Client --


bool ServiceCache::Key::operator<(const Key& o) const
{
	if(urn != o.urn)
		return urn < o.urn;
	return o.location < o.location;
}


Client::Client(
		boost::asio::io_context& io_context,
		const boost::asio::ip::address_v4& interfaceAddress,	///< Address of the interface to listen to
		callback_t notify):
	m_connection(io_context, interfaceAddress, [this](auto msg){ this->OnReceive(msg); }),
	m_notify(notify)
{
}


void Client::Search(std::string urn)
{
	Ssdp::Search search;
	search.searchType = urn;

	// TO-DO: Lookup in m_services, send those notifies immediately
	// Then, send the query to discover potential new devices

	std::string ss = search;
	m_connection.send(ss);
}


void Client::OnReceive(std::string_view msg)
{
	auto sn = Parse(msg);
	if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
	{
		m_notify(*pNotify);

		ServiceCache::Key key{pNotify->nt, pNotify->location};
		m_cache.services[key] = *pNotify;
	}
}


//-- Bridge --


Bridge::Bridge(boost::asio::io_context& io_context, Settings settings):
	m_Client(io_context, boost::asio::ip::make_address_v4(settings.client.ip), [this](auto msg){ this->OnReceiveFromClient(msg); }),
	m_Server(io_context, boost::asio::ip::make_address_v4(settings.server.ip), [this](auto msg){ this->OnReceiveFromServer(msg); })
{
}


void Bridge::OnReceiveFromClient(const std::string_view& data)
{
	auto sn = Parse(data);
	if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
	{
		ServiceCache::Key key{pNotify->nt, pNotify->location};
		if(m_cache.services.count(key) == 0)
			std::cout << "New service from client:\t" << pNotify->nt << std::endl;
		m_cache.services[key] = *pNotify;
	}
	else if(auto pSearch = std::get_if<Ssdp::Search>(&sn))
		std::cout << "Search from client:\t" << pSearch->searchType << std::endl;

	m_Server.send(data);
}


void Bridge::OnReceiveFromServer(const std::string_view& data)
{
	auto sn = Parse(data);
	if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
	{
		ServiceCache::Key key{pNotify->nt, pNotify->location};
		if(m_cache.services.count(key) == 0)
			std::cout << "New service from server:\t" << pNotify->nt << std::endl;
		m_cache.services[key] = *pNotify;
	}

	m_Client.send(data);
}


} // namespace Ssdp
