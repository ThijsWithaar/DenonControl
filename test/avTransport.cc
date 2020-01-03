#include <catch2/catch.hpp>

#include <Denon/network/ssdp.h>
#include <Denon/upnpControl.h>

#include "dummyConnection.h"

#include "avTransport.dump.h"


#ifdef HAS_BOOST_BEAST
TEST_CASE("Capture AvTransport", "[!hide]")
{
	boost::asio::io_context ioc;
	Denon::Http::BeastConnection conn(ioc, "192.168.4.7", Ssdp::port);
	conn.setCaptureFilename("avTransport.dump");

	Denon::Upnp::AvTransport avt(&conn);
	avt.GetCurrentState();
	avt.GetCurrentTransportActions();
	avt.GetDeviceCapabilities();
	avt.GetMediaInfo();
	avt.GetPositionInfo();
	avt.GetTransportInfo();
	avt.GetTransportSettings();
}
#endif // HAS_BOOST_BEAST


TEST_CASE("Parse AvTransport")
{
	using namespace Denon::Upnp;
	DummyConnection conn({(const char*)g_avTransport, sizeof(g_avTransport)});
	AvTransport avt(&conn);

	auto cs = avt.GetCurrentState();
	avt.GetCurrentTransportActions();
	avt.GetDeviceCapabilities();
	avt.GetMediaInfo();
	avt.GetPositionInfo();
	avt.GetTransportInfo();
	avt.GetTransportSettings();
}
