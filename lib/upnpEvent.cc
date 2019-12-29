#include <Denon/upnpEvent.h>

#include <cstring>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Denon/string.h>
#include <Denon/http.h>


namespace Denon {
namespace Upnp {


Subscribe::operator std::string()
{
	Http::BlockingConnection::Request r;
	r.method = Http::BlockingConnection::Method::Subscribe;
	r.path = url;
	r.fields["HOST"] = host;
	r.fields["CALLBACK"] = callback;
	r.fields["NT"] = type;
	r.fields["TIMEOUT"] = timeOut;
	return r;
}


void EventParser::operator()(std::string_view body, EventHandler& handler)
{
	std::stringstream bodyss;
	bodyss << urlDecode(body);
	//std::cout << "Upnp::EventParser decoded:\n" << bodyss.str() << "\n";

	boost::property_tree::ptree pt;
	boost::property_tree::read_xml(bodyss, pt);

	if(auto events = pt.get_child_optional("e:propertyset.e:property.LastChange.Event"))
	{
		for(auto& event: *events)
		{
			parseEvents(event.first, event.second, handler);
		}
	}
}


void EventParser::parseEvents(const std::string& name, const boost::property_tree::ptree& pt, EventHandler& handler)
{
	if(name == "FriendlyName")
	{
		auto val = pt.get<std::string>("<xmlattr>.val");
		handler.onDeviceName(val);
	}
	else if(name == "DevicePower")
	{
		auto val = pt.get<std::string>("<xmlattr>.val");
		handler.onPower(val == "ON");
	}
	else if(name == "InstanceID")
	{
		for(auto& [key, val]: pt)
		{
			if(key == "Volume")
			{
				auto channel = val.get<std::string>("<xmlattr>.channel");
				auto vval = val.get<double>("<xmlattr>.val");
				if(channel == "Master")
					handler.onVolume(Denon::Channel::Master, vval);
				else
					handler.onZoneVolume(channel, vval);
			}
			else if(key == "Bass")
				handler.onVolume(Denon::Channel::Bass, val.get<double>("<xmlattr>.val"));
			else if(key == "Treble")
				handler.onVolume(Denon::Channel::Treble, val.get<double>("<xmlattr>.val"));
			else if(key == "Subwoofer")
				handler.onVolume(Denon::Channel::Sub, val.get<double>("<xmlattr>.val"));
			else if(key == "Mute")
			{
				auto channel = val.get<std::string>("<xmlattr>.channel");
				auto mval = val.get<int>("<xmlattr>.val");
				handler.onMute(channel, mval != 0);
			}
		}
	}
	else if(name == "WifiApSsid")
	{
		auto mval = pt.get<std::string>("<xmlattr>.val");
		handler.wifiSsid(mval);
	}
	else if(name == "SurroundSpeakerConfig")
	{
		/*auto cfg = pt.get<std::string>("<xmlattr>.val");
		std::stringstream ss;
		ss << urlDecode(cfg);
		std::cout << "* SurroundSpeakerConfig *\n" << ss.str() << "\n";*/
	}
}


} // namespace Upnp
} // namespace Denon
