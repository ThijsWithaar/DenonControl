#include <Denon/network/ssdp.h>

#include <iostream>
#include <filesystem>

#ifdef __linux__
	#include <ifaddrs.h>
	#include <pwd.h>
#endif

#include <boost/asio/local/stream_protocol.hpp>

#include <Denon/network/http.h>
#include <Denon/string.h>


#define DEBUG_LVL 2



namespace Ssdp {


constexpr short multicast_port = 1900;

const boost::asio::ip::address_v4 multicast_address = boost::asio::ip::make_address_v4("239.255.255.250");
const boost::asio::ip::address_v6 multicast_address6 = boost::asio::ip::make_address_v6("FF02::C");


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


std::string GetConfigurationPath(std::string fname, bool system)
{
	const char *homedir;
	if((homedir = getenv("HOME")) == nullptr)
		homedir = getpwuid(getuid())->pw_dir;

	std::string sys = "/etc/" + fname;
	std::string user = std::string(homedir) + "/." + fname;

	if(std::filesystem::exists(user) && !system)
		return user;

	return sys;
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


#else


std::string GetConfigurationPath(std::string fname, bool create)
{
	return {};
}


std::vector<Interface> GetNetworkInterfaces()
{
	return {};
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
	Denon::Http::Request r;
	r.method = Denon::Http::Method::MSearch;
	r.path = "*";

	r.fields = custom;
	r.fields["HOST"] = "239.255.255.250:1900";
	r.fields["MAN"] = "\"ssdp:discover\"";
	r.fields["MX"] = "10";
	r.fields["ST"] = searchType;
	return r;
}


bool Search::operator==(const Search& o) const
{
	return searchType == o.searchType && custom == o.custom;
}


Response::operator std::string()
{
	Denon::Http::Response r;
	r.status = 200;
	r.fields = custom;
	r.fields["ST"] = nt;
	return r;
}


bool Response::operator==(const Response& o) const
{
	return location == o.location && nt == o.nt && custom == o.custom;
}


Notify::Notify(const Response& r):
	Response(r)
{
}


Notify::operator std::string()
{
	Denon::Http::Request r;
	r.method = Denon::Http::Method::Notify;
	r.path = "*";
	r.fields = {
		{"HOST", "239.255.255.250:1900"},
		{"NT", nt},
		{"LOCATION", location}
	};
	r.fields.insert(custom.begin(), custom.end());
	return r;
}


bool Notify::operator==(const Notify& o) const
{
	return Response::operator==(o);
}


Notify ParseNotify(std::string_view message)
{
	Notify n;
	std::string dummy;
	QueryEntries qe = {
		{"LOCATION", n.location},
		{"NT", n.nt},
		{"HOST", dummy},
	};

	ParseSddp(message, qe, n.custom);
	return n;
}


Response ParseResponse(std::string_view message)
{
	Notify n;
	QueryEntries qe = {
		{"LOCATION", n.location},
		{"ST", n.nt},
	};

	ParseSddp(message, qe, n.custom);
	return n;
}


std::variant<Search, Response, Notify> Parse(std::string_view message)
{
	if(startswith(message, "M-SEARCH"))
		return ParseSearch(message);
	else if(startswith(message, "NOTIFY"))
		return ParseNotify(message);
	else if(startswith(message, "HTTP"))
		return ParseResponse(message);
	throw std::runtime_error("SSDP: Unhandled message type");
}



//-- Connection --


Connection::Base::Base(
		boost::asio::io_context& context,
		const boost::asio::ip::address_v4& itf,
		rx_callback_t onRx):
	m_socket(context),
	m_itf(itf),
	m_cb(onRx)
{
	// Create the socket so that multiple may be bound to the same address.
	m_socket.open(boost::asio::ip::udp::v4());
	m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
}


void Connection::Base::send(boost::asio::ip::udp::endpoint dst, const std::string_view& data)
{
	// TO-DO: mutex on m_txBuf, release in the lambda
	std::copy(data.begin(), data.end(), m_txBuf.begin());

	m_socket.async_send_to(
		boost::asio::buffer(m_txBuf),
		dst,
		[&](const boost::system::error_code& error, std::size_t bytes_transferred )
		{
		}
	);
}


void Connection::Base::ScheduleRx()
{
	m_socket.async_receive_from(
		boost::asio::buffer(m_rxBuf.data(), m_rxBuf.size()),
		m_sender_endpoint,
		[&](const boost::system::error_code& error, size_t bytes_recvd)
		{
			// Since linux does not do UDP interface filtering on multicast, do it by hand:
			auto ipS = m_sender_endpoint.address().to_v4();
			if(sameSubnet(ipS, m_itf, 24) && (ipS != m_itf))
			{
				//std::cout << "RX from " << ipS << ", interface " << ipItf << "\n";
				std::string_view msg(m_rxBuf.data(), bytes_recvd);
				m_cb(ipS, msg);
			}
			ScheduleRx();
		}
	);
}


//-- Connection::MultiCast --


Connection::MultiCast::MultiCast(
			boost::asio::io_context& context,
			const boost::asio::ip::address_v4& itf,
			rx_callback_t onRx):
		Base(context, itf, onRx)
{
	m_socket.bind({multicast_address, multicast_port});

	// Join the multicast group
	// Limiting to the interfaceAddress works in windows, not in linux.
	// https://stackoverflow.com/questions/31101047/getting-the-destination-address-of-udp-packet
	m_socket.set_option(boost::asio::ip::multicast::join_group(multicast_address, itf));

	// Limit sending to only this interface
	m_socket.set_option(boost::asio::ip::multicast::outbound_interface( itf ));

	Base::ScheduleRx();
}


void Connection::MultiCast::broadcast(const std::string_view& data)
{
	send({multicast_address, multicast_port}, data);
}


//-- Connection::Udp --


Connection::Listen::Listen(
		boost::asio::io_context& context,
		const boost::asio::ip::address_v4& itf,
		rx_callback_t onRx):
	Base(context, itf, onRx)
{
	m_socket.bind({itf, multicast_port});
	Base::ScheduleRx();
}


void Connection::Listen::send(const boost::asio::ip::address_v4& dst, const std::string_view& data)
{
	Base::send({dst, multicast_port}, data);
}


//-- Connection --


Connection::Connection(
			boost::asio::io_context& io_context,
			const boost::asio::ip::address_v4& interfaceAddress,
			rx_callback_t onReceive
			):
		m_multicast(io_context, interfaceAddress, onReceive),
		m_listen(io_context, interfaceAddress, onReceive)
{
	#if DEBUG_LVL > 0
		std::cout << "SSDP Connection on interface: " << interfaceAddress.to_string() << std::endl;
	#endif
}


void Connection::broadcast(const std::string_view& data)
{
	m_multicast.broadcast(data);
}


void Connection::send(const boost::asio::ip::address_v4& dst, const std::string_view& data)
{
	m_listen.send(dst, data);
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
	m_connection(io_context, interfaceAddress, [this](auto sender, auto msg){ this->OnReceive(sender, msg); }),
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
	m_connection.broadcast(ss);
}


void Client::OnReceive(boost::asio::ip::address_v4 sender, std::string_view msg)
{
	auto sn = Parse(msg);
	if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
	{
		m_notify(sender, *pNotify);

		ServiceCache::Key key{pNotify->nt, pNotify->location};
		m_cache.services[key] = *pNotify;
	}
}


} // namespace Ssdp
