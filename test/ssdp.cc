#include <catch2/catch.hpp>

#include <Denon/ssdp.h>


// 130 bytes
constexpr auto searchMedia = R"(M-SEARCH * HTTP/1.1
HOST: 239.255.255.250:1900
MAN: "ssdp:discover"
MX: 10
ST: urn:schemas-upnp-org:device:MediaRenderer:1
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


TEST_CASE("Parse Search")
{
	SECTION("media")
	{
		auto sn = Ssdp::Parse(searchMedia);
		REQUIRE(std::holds_alternative<Ssdp::Search>(sn));
		auto s = std::get<Ssdp::Search>(sn);
		CHECK(s.searchType == "urn:schemas-upnp-org:device:MediaRenderer:1");
	}

	SECTION("denon")
	{
		auto sn = Ssdp::Parse(searchActDenon);
		REQUIRE(std::holds_alternative<Ssdp::Search>(sn));
		auto s = std::get<Ssdp::Search>(sn);
		CHECK(s.searchType == "urn:schemas-denon-com:device:ACT-Denon:1");
	}
}


TEST_CASE("Parse Notify")
{
	SECTION("con man")
	{
		auto sn = Ssdp::Parse(notifyConnectionManager);
		REQUIRE(std::holds_alternative<Ssdp::Notify>(sn));
		auto n = std::get<Ssdp::Notify>(sn);
		CHECK(n.nt == "urn:schemas-upnp-org:service:ConnectionManager:1");
		CHECK(n.location == "http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml");
	}
}
