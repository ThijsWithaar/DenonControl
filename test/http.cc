#include <catch2/catch_all.hpp>
//#include <catch2/catch_test_macros.hpp>

#include <iostream>

#include <Denon/network/http.h>
#include <Denon/network/ssdp.h>
#include <Denon/upnpControl.h>

#include "dummyConnection.h"
#include "blockingHttp.dump.h"


#ifdef HAS_BOOST_BEAST

// This generates the capture used for the tests below
TEST_CASE("Capture Control", "[.][dump]")
{
	using namespace Denon::Upnp;

	boost::asio::io_context ioc;
	Denon::Http::BeastConnection conn(ioc, "192.168.4.7", Ssdp::port);
	conn.setCaptureFilename("blockingHttp.dump");

	DenonAct denon(&conn);
	//denon.GetAccessPointList();
	denon.GetAudioConfig();
	denon.GetAvrZoneStatus();
	denon.GetSurroundSpeakerConfig();
	denon.GetCurrentState();

	RenderingControl rc(&conn);
	rc.GetCurrentState();
	rc.GetMute(RenderingControl::Channel::Master);
}
#endif // HAS_BOOST_BEAST



TEST_CASE("Http parsing")
{
	using namespace Denon::Upnp;

	DummyConnection conn({(const char*)g_blockingHttp, sizeof(g_blockingHttp)});
	Denon::Http::Parser parser(Denon::Http::Method::Get);

	auto dst = parser.currentBuffer();
	auto nRead = conn.read(dst.first, dst.second);
	auto pAudioCfg = parser.markRead(nRead);
	REQUIRE(pAudioCfg != nullptr);
	auto ptAudioCfg = Denon::Upnp::DecodeXmlResponse(*pAudioCfg, "u:GetAudioConfigResponse.AudioConfig");
	auto audioCfg = ParseAudioConfig(ptAudioCfg);
	CHECK(audioCfg.lowpass == 80);

	dst = parser.currentBuffer();
	nRead = conn.read(dst.first, dst.second);
	auto pAvrZoneStatus = parser.markRead(nRead);
	REQUIRE(pAvrZoneStatus != nullptr);
	auto ptZoneStatus = Denon::Upnp::DecodeXmlResponse(*pAvrZoneStatus, "u:GetAvrZoneStatusResponse.status");
	auto zoneStatus = ParseZoneStatus(ptZoneStatus);
	REQUIRE(zoneStatus.zones.size() == 2);
}


TEST_CASE("Control parsing")
{
	using namespace Denon::Upnp;

	DummyConnection conn({(const char*)g_blockingHttp, sizeof(g_blockingHttp)});
	DenonAct denon(&conn);

	// Re-play the request executed to capture 'upnpControlDump[]';
	auto audioCfg = denon.GetAudioConfig();
	CHECK(audioCfg.lowpass == 80);

	auto zoneStatus = denon.GetAvrZoneStatus();
	REQUIRE(zoneStatus.zones.size() == 2);

	auto speakerConfig = denon.GetSurroundSpeakerConfig();
	CHECK(speakerConfig.front.left.distance == 12);

	auto cState = denon.GetCurrentState();
	CHECK(cState.friendlyName == "Thijs' Denon");

	RenderingControl rc(&conn);
	auto rState = rc.GetCurrentState();
	CHECK(rState.balance == 50.);
	CHECK(rState.presetNames == std::vector<std::string>{
		"FactoryDefaults","InstallationDefaults","Disabled","Accoustic","Classical", "Jazz","Pop","Rock"
	});
}
