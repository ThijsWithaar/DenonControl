#include "MainWindow.h"

#include <iostream>

#include <Denon/appInterface.h>
#include <Denon/ssdp.h>

#include <denonVersion.h>



int main(int argc, char* argv[])
{
#if 0
	Denon::AppInterface denon("192.168.4.7");
	Denon::Command cmd(&denon);

	auto di = denon.GetDeviceInfo();
	cmd.Mute(true);

	auto ds = denon.GetDeviceStatus();
	std::cout << ds.surround << std::endl;
	return 4;

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
