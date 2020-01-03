// Interface used by the android app.
#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include <Denon/network/http.h>
#include <Denon/serial.h>


namespace Denon {


// https://blue-pc.net/2013/12/28/denon-av-reciever-ueber-http-steuern/
//
// Connect to
//	http://192.168.4.7:8080/goform/Deviceinfo.xml

struct DeviceCapabilities
{
};

struct ZoneCapabilities
{
};

struct DeviceInfo
{
	int version;
	int commApiVersion;
	std::string catName;
	std::string modelName;
	std::string manualModelName;
	std::string mac;
	DeviceCapabilities capabilities;
	std::vector<ZoneCapabilities> zoneCapabilities;
};

struct ZoneStatus
{
	std::string power;
	std::string source;
	double volume;
	std::string mute;
};

struct DeviceStatus
{
	std::map<std::string, ZoneStatus> zones;
	std::string surround;
};

class AppInterface:
	public CommandConnection
{
public:
	//AppInterface(std::string host, int port=8080);
	AppInterface(std::unique_ptr<Http::BlockingConnection>&& con);

	~AppInterface() = default;

	DeviceInfo GetDeviceInfo();

	DeviceStatus GetDeviceStatus();

	/// Movie, Pure, Music, Game
	void SetSoundMode(int mode);

	/// CommandConnection
	void Send(const std::string&) override;

private:
	const Http::Response& Get(std::string path);
	const Http::Response& Post(std::string path, std::string body);

	boost::property_tree::ptree Parse(const Http::Response&);

	// Application Command
	const Http::Response& AppCommand(std::vector<std::string> attributes);
	const Http::Response& AppCommand3(std::vector<boost::property_tree::ptree> commands);

	std::unique_ptr<Http::BlockingConnection> m_http;
};


} // namespace Denon
