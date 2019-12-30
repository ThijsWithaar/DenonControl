#include "MainWindow.h"

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Denon/appInterface.h>
#include <Denon/http.h>
#include <Denon/scdp.h>
#include <Denon/ssdp.h>
#include <Denon/string.h>
#include <Denon/upnpControl.h>

#include <denonVersion.h>

#include "blockingHttp.h"



int main(int argc, char* argv[])
{
#if 0
	//-- Test Android API --
	Denon::AppInterface denon("192.168.4.7");
	Denon::Command cmd(&denon);

	auto di = denon.GetDeviceInfo();
	cmd.Mute(true);

	auto ds = denon.GetDeviceStatus();
	std::cout << ds.surround << std::endl;
	return 4;
#elif 0
	//-- Test SCDP --
	#if 0
		QHttpConnection hcon("192.168.4.7", Ssdp::port);
		auto resp = hcon.Http({Denon::Http::Method::Get, Ssdp::path});
		Denon::SCDP scdp(resp.body);

		for(auto dev: scdp.devices)
		{
			std::cout << "Device " << dev.type << "\n";
			for(auto ser: dev.services)
				std::cout << "\tService " << ser.eventUrl << "\n";
		}
	#endif

	boost::asio::io_context ioc;
	//Denon::Http::BeastConnection hcon2(ioc, "192.168.4.7", Ssdp::port);
	QHttpConnection hcon2("192.168.4.7", Ssdp::port);

	Denon::Upnp::DenonAct act(&hcon2);
	//auto ac = act.GetAudioConfig(); std::cout << ac.soundMode << "\n";
	//auto azs = act.GetAvrZoneStatus(); std::cout << azs.zones.front().name << "\n";
	act.GetSurroundSpeakerConfig();

	Denon::Upnp::RenderingControl rc(&hcon2);
	//std::cout << rc.GetCurrentState().subwoofer << "\n";
	//auto mt = rc.GetMute(RenderingControl::Channel::Master); std::cout << "\n= GetMute\n'" << mt << "'\n";

#elif 1
	QApplication app(argc, argv);

	// This determines the filename of the settings
	app.setApplicationName(PROJECT_NAME);
	app.setOrganizationName(PACKAGE_VENDOR);
	app.setOrganizationDomain("withaar.net");
	app.setApplicationVersion(PROJECT_VERSION);

	MainWindow main(nullptr);
	main.show();
	return app.exec();
#endif
}
