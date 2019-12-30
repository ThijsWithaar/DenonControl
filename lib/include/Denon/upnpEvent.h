#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>

#include <boost/property_tree/ptree.hpp>

#include <Denon/denon.h>


namespace Denon {
namespace Upnp {


/// Subscribe message
struct Subscribe
{
	std::string url;		///< upnp/event/renderer_dvc/ConnectionManager
	std::string host;		///< 192.168.4.7:60006
	std::string callback;	///< <http://192.168.3.11:49200/>
	std::string type;		///< upnp:event
	std::string timeOut;	///< Second-180

	operator std::string();
};

class EventHandler
{
public:
	virtual void onDeviceName(std::string_view name) = 0;
	virtual void onPower(bool on) = 0;
	virtual void onVolume(Denon::Channel c, double vol) = 0;
	virtual void onZoneVolume(std::string_view name, double vol) = 0;
	virtual void onMute(std::string_view channel, bool muted) = 0;
	virtual void wifiSsid(std::string_view ssid) = 0;
};

class EventParser
{
public:
	void operator()(std::string_view body, EventHandler& handler);
private:
	void parseEvents(const std::string& name, const boost::property_tree::ptree& pt, EventHandler& handler);
};

} // namespace Upnp
} // namespace Denon
