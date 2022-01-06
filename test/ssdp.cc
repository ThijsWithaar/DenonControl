#include <catch2/catch.hpp>
//#include <catch2/catch_all.hpp>

#include <iostream>

#include <boost/asio/local/stream_protocol.hpp>

#include <Denon/network/http.h>
#include <Denon/network/ssdp.h>
#include <Denon/network/ssdpIpc.h>



#include "ssdp.dump.h"


// 130 bytes
constexpr auto searchMedia = R"(M-SEARCH * HTTP/1.1
HOST: 239.255.255.250:1900
MAN: "ssdp:discover"
MX: 10
ST: urn:schemas-upnp-org:device:MediaRenderer:1
)";


constexpr auto searchResponse = R"(HTTP/1.1 200 OK
CACHE-CONTROL: max-age=180
EXT:
LOCATION: http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
VERSIONS.UPNP.HEOS.COM: 10,21230580,-521045671,363364703,1840750642,105553199,-316033077,1711326982,395144743,-170053632,363364703
NETWORKID.UPNP.HEOS.COM: b827eb4ccfb5
BOOTID.UPNP.ORG: 344425180
IPCACHE.URL.UPNP.HEOS.COM: /ajax/upnp/get_device_info
SERVER: LINUX UPnP/1.0 Denon-Heos/150495
ST: urn:schemas-upnp-org:device:MediaServer:1
USN: uuid:4c8f081e-802e-1c18-d77c-423dae53ee1a::urn:schemas-upnp-org:device:MediaServer:1
)";


// 127 bytes
constexpr auto searchActDenon = R"(M-SEARCH * HTTP/1.1
HOST: 239.255.255.250:1900
MAN: "ssdp:discover"
MX: 10
ST: urn:schemas-denon-com:device:ACT-Denon:1

)";


constexpr auto notifyConnectionManager = R"(NOTIFY * HTTP/1.1
HOST: 239.255.255.250:1900
CACHE-CONTROL: max-age=180
LOCATION: http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
VERSIONS.UPNP.HEOS.COM: 10,-592772649,-521045671,363364703,1840750642,105553199,-316033077,1711326982,-170053632,363364703,395144743
NETWORKID.UPNP.HEOS.COM: b827eb4ccfb5
BOOTID.UPNP.ORG: 344425142
IPCACHE.URL.UPNP.HEOS.COM: /ajax/upnp/get_device_info
NT: urn:schemas-upnp-org:service:ConnectionManager:1
NTS: ssdp:alive
SERVER: LINUX UPnP/1.0 Denon-Heos/150495
USN: uuid:c67759b4-4f58-10c3-0080-0005cdb04ece::urn:schemas-upnp-org:service:ConnectionManager:1
)";


// 602 bytes
constexpr auto notifyActDenon = R"(NOTIFY * HTTP/1.1
HOST: 239.255.255.250:1900
CACHE-CONTROL: max-age=180
LOCATION: http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
VERSIONS.UPNP.HEOS.COM: 10,-592772649,-521045671,363364703,1840750642,105553199,-316033077,1711326982,-170053632,363364703,395144743
NETWORKID.UPNP.HEOS.COM: b827eb4ccfb5
BOOTID.UPNP.ORG: 344425142
IPCACHE.URL.UPNP.HEOS.COM: /ajax/upnp/get_device_info
NT: urn:schemas-denon-com:device:ACT-Denon:1
NTS: ssdp:alive
SERVER: LINUX UPnP/1.0 Denon-Heos/150495
USN: uuid:388fa6d1-6a13-f5df-4c59-47b673b2f9a7::urn:schemas-denon-com:device:ACT-Denon:1
)";


// Received from Server 616 bytes
constexpr auto notifyConMan = R"(NOTIFY * HTTP/1.1
HOST: 239.255.255.250:1900
CACHE-CONTROL: max-age=180
LOCATION: http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
VERSIONS.UPNP.HEOS.COM: 10,21230580,-521045671,363364703,1840750642,105553199,-316033077,1711326982,395144743,-170053632,363364703
NETWORKID.UPNP.HEOS.COM: b827eb4ccfb5
BOOTID.UPNP.ORG: 344425143
IPCACHE.URL.UPNP.HEOS.COM: /ajax/upnp/get_device_info
NT: urn:schemas-upnp-org:service:ConnectionManager:1
NTS: ssdp:alive
SERVER: LINUX UPnP/1.0 Denon-Heos/150495
USN: uuid:4c8f081e-802e-1c18-d77c-423dae53ee1a::urn:schemas-upnp-org:service:ConnectionManager:1
)";




namespace std {
template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, std::map<Key, Value> map)
{
	for(auto& it: map)
		os << "{" << it.first << " : " << it.second << "}, ";
	return os;
}
} // std


namespace Ssdp {
std::ostream& operator<<(std::ostream& os, Ssdp::Notify n)
{
	return os << "type: " << n.nt << ", loc: " << n.location << ", fields: [ " << n.custom << " ]\n";
}
}


#ifdef HAS_BOOST_BEAST
TEST_CASE("Capture SSDP", "[.][dump]")
{
	const std::string discoverIp = "192.168.3.10";
	auto itf = boost::asio::ip::make_address_v4(discoverIp);

	boost::asio::io_context ioc;

	std::ofstream fdump("ssdp.dump");
	auto onRx = [&](boost::asio::ip::address_v4 sender, std::string_view msg)
	{
		std::cout << "Received " << msg.size() << " bytes from " << sender.to_string() << "\n";
		fdump.write(msg.data(), msg.size());
	};

	Ssdp::Connection con(ioc, itf, onRx);

	std::cout << "Listening" << std::endl;
	using namespace std::chrono_literals;
	ioc.run_for(60s);

	std::cout << "Exiting" << std::endl;
}


TEST_CASE("Run SSDP Client", "[.][manual]")
{
	auto itf = boost::asio::ip::make_address_v4("192.168.3.10");
	boost::asio::io_context ioc;

	std::vector<Ssdp::Notify> notifies;
	auto notify = [&notifies](boost::asio::ip::address_v4 sender, Ssdp::Notify f)
	{
		std::cout << "Notified " << f.location << " from " << sender.to_string() << "\n";
		notifies.push_back(f);
		return;
	};

	Ssdp::Client client(ioc, itf, notify);
	client.Search("ssdp:all");
	client.Search("urn:schemas-upnp-org:device:MediaRenderer:1");

	using namespace std::chrono_literals;
	ioc.run_for(30s);
}
#endif



TEST_CASE("MiniSSDP Client", "[.][manual]")
{
	boost::asio::io_context ioc;

	// Start the server (otherwise, use minissdp)
	MiniSsdp::Server::Settings ipcSettings;
	Ssdp::ServiceCache cache;
#if 0
	ipcSettings.socketName = "minissdp.sock";
	MiniSsdp::Server server(ioc, cache, ipcSettings);
#endif

	// Send a notify
	Ssdp::Notify nt;
	nt.location = "http://192.168.3.10";
	nt.nt = "urn:schemas-denon-com:device:Dummy:1";
	nt.custom["USN"] = "uuid:b00b13s-4f58-10c3-0080-0005cdb04ece::urn:schemas-upnp-org:service:Dummy:1";
	nt.custom["SERVER"] = "Denon Unit test";

	{
		auto home = boost::asio::ip::make_address_v4("192.168.3.10");
		//auto home = boost::asio::ip::make_address_v4("127.0.0.1");
		Ssdp::Connection con(ioc, home, [](auto s, auto msg){});

		std::string msg = nt;
		con.broadcast(msg);		// These are received, but don't seem to make it into the cache
		con.broadcast(notifyConnectionManager);
		con.broadcast(notifyActDenon);
		//con.send(home, msg);
	}

	auto ntCon = std::get<Ssdp::Notify>(Ssdp::Parse(notifyConnectionManager));
	auto ntDenon = std::get<Ssdp::Notify>(Ssdp::Parse(notifyActDenon));

	MiniSsdp::Client client(ioc, ipcSettings);

	client.Submit(ntCon);
	client.Submit(ntDenon);
	client.Submit(nt);

	auto ver = client.GetVersion();
	CHECK(ver.size() > 0);
	auto ver2 = client.GetVersion();
	CHECK(ver2.size() > 0);
	//std::cout << "Version : " << ver << "\n";

	auto conUsn = client.SearchUsn(ntCon.custom["USN"]);
	CHECK(conUsn.size() == 2);

	auto denUsn = client.SearchUsn(ntDenon.custom["USN"]);
	CHECK(denUsn.size() == 2);

	client.SearchType(nt.nt);
	auto all = client.RequestAll();
	CHECK(all.size() > 3);
}


TEST_CASE("SSDP Parse")
{
	SECTION("search MediaRenderer")
	{
		auto sn = Ssdp::Parse(searchMedia);
		REQUIRE(std::holds_alternative<Ssdp::Search>(sn));

		auto s = std::get<Ssdp::Search>(sn);
		CHECK(s.searchType == "urn:schemas-upnp-org:device:MediaRenderer:1");
	}

	SECTION("response MediaServer")
	{
		auto sn = Ssdp::Parse(searchResponse);
		REQUIRE(std::holds_alternative<Ssdp::Response>(sn));

		auto r = std::get<Ssdp::Response>(sn);
		CHECK(r.nt == "urn:schemas-upnp-org:device:MediaServer:1");
	}

	SECTION("Search Denon")
	{
		auto sn = Ssdp::Parse(searchActDenon);
		REQUIRE(std::holds_alternative<Ssdp::Search>(sn));

		auto s = std::get<Ssdp::Search>(sn);
		CHECK(s.searchType == "urn:schemas-denon-com:device:ACT-Denon:1");
	}

	SECTION("Notify: ConnectionManager")
	{
		auto sn = Ssdp::Parse(notifyConnectionManager);
		REQUIRE(std::holds_alternative<Ssdp::Notify>(sn));

		auto n = std::get<Ssdp::Notify>(sn);
		CHECK(n.nt == "urn:schemas-upnp-org:service:ConnectionManager:1");
		CHECK(n.location == "http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml");
	}
}


TEST_CASE("SSDP generate")
{
	SECTION("Search")
	{
		Ssdp::Search search;
		search.searchType = "urn:schemas:xx:1";

		std::string searchStr = search;
		CHECK(Catch::startsWith(searchStr, "M-SEARCH * HTTP/1.1\r\n"));

		auto sn = Ssdp::Parse(searchStr);
		REQUIRE(std::holds_alternative<Ssdp::Search>(sn));

		auto searchTest = std::get<Ssdp::Search>(sn);
		CHECK(search.searchType == searchTest.searchType);
	}

	SECTION("Response")
	{
		Ssdp::Response response;
		response.location = "http://127.0.0.1";

		std::string responseStr = response;
		CHECK(Catch::startsWith(responseStr, "HTTP/1.1 200 OK\r\n"));

		auto sn = Ssdp::Parse(responseStr);
		REQUIRE(std::holds_alternative<Ssdp::Response>(sn));
	}

	SECTION("Notify")
	{
		Ssdp::Notify notify;
		notify.location = "http://127.0.0.1";
		notify.nt = "urn:schemas:xx:1";
		notify.custom["CONTENT-LENGTH"] = "0";

		std::string notifyStr = notify;
		CHECK(Catch::startsWith(notifyStr, "NOTIFY * HTTP/1.1\r\n"));

		auto sn = Ssdp::Parse(notifyStr);
		REQUIRE(std::holds_alternative<Ssdp::Notify>(sn));

		auto notifyTest = std::get<Ssdp::Notify>(sn);
		CHECK(notify == notifyTest);
	}
}
