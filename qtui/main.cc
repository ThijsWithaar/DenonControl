#include "MainWindow.h"

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Denon/network/http.h>
#include <Denon/network/ssdp.h>
#include <Denon/network/scdp.h>
#include <Denon/appInterface.h>
#include <Denon/string.h>
#include <Denon/upnpControl.h>

#include <denonVersion.h>

#include "blockingHttp.h"
#include "QExceptionApplication.h"



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

#elif 1
	QExceptionApplication app(argc, argv, "Denon Control");

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
