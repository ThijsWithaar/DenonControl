#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace Denon {


/// Parse a SCDP control description (from xml)
class SCDP
{
public:
	struct Service
	{
		std::string type, id;
		std::string scdpUrl, controlUrl, eventUrl;
	};

	struct Device
	{
		std::string type;
		std::string friendlyname, modelName, modelNumber;
		std::string udn;
		std::vector<Service> services;
	};

	struct Audyssey
	{
		int version;
		int port;
		int webApiPort;
	};

	/// Parse from xml data.
	/// e.g. from body of http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
	SCDP(std::string_view xml);

	Device root;
	Audyssey audyssey;
	std::vector<Device> devices;
};


} // namespace Denon
