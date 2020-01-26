#include <Denon/upnpEvent.h>

#include <cstring>
#include <iostream>
#include <optional>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Denon/string.h>
#include <Denon/network/http.h>
#include <Denon/upnpControl.h>


namespace Denon {
namespace Upnp {


Subscribe::operator std::string()
{
	Http::Request r;
	r.method = Http::Method::Subscribe;
	r.path = url;
	r.fields["HOST"] = host;
	r.fields["CALLBACK"] = callback;
	r.fields["NT"] = type;
	r.fields["TIMEOUT"] = timeOut;
	return r;
}


void EventParser::operator()(std::string_view body, EventHandler& handler)
{
	auto pt = ParseXml(body);

	auto lastChange = pt.get_optional<std::string>("e:propertyset.e:property.LastChange");
	if(!lastChange)
		return;

	auto ptLC = ParseXml(*lastChange);
	if(auto events = ptLC.get_child_optional("Event"))
	{
		for(auto& event: *events)
		{
			parseEvents(event.first, event.second, handler);
		}
	}
}


void EventParser::parseEvents(const std::string& name, const boost::property_tree::ptree& pt, EventHandler& handler)
{
	std::optional<RenderingControl::CurrentState> rcCs;

	auto val = pt.get_optional<std::string>("<xmlattr>.val");
	//std::cout << "EventParser::parseEvents " << name << "\n";
	if(name == "AudioConfig")
	{
		DenonAct::AudioConfig ac = ParseAudioConfig(ParseXml(*val));
	}
	else if(name == "AvrZoneStatus")
	{
		DenonAct::ZoneStatus zs = ParseZoneStatus(ParseXml(*val));
	}
	else if(name == "BTConfig")
	{
		DenonAct::BluetoothConfig bc = ParseBtConfig(ParseXml(*val));
	}
	else if(name == "FriendlyName")
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
		// See also: RenderingControl::GetCurrentState
		for(auto& [key, val]: pt)
		{
			if(key == "Volume" && !rcCs.has_value())
			{
				rcCs = ParseRenderingControlState(pt);
			}
			else if(key == "AVTransportURI")
			{
				auto ats = ParseAvTransportState(pt);
				//std::cout << "AvTransportState:\n" << ats << "\n";
			}
			else if(key == "<xmlattr>")
			{
			}
			else
			{
				//std::cerr << "EventParser: Unimplemented key " << key << "\n";
			}
		}
	}
	else if(name == "NetworkConfigurationList")
	{
		std::vector<DenonAct::NetworkConfiguration> ncs = ParseNetworkConfigs(ParseXml(*val));
	}
	else if(name == "WifiApSsid")
	{
		handler.wifiSsid(*val);
	}
	else if(name == "SurroundSpeakerConfig")
	{
		DenonAct::SpeakerConfig cfg = ParseSpeakerConfig(ParseXml(*val));
		std::cout << "SurroundSpeakerConfig:\n" << cfg << "\n";
	}

	if(rcCs.has_value())
	{
		// This sends all volume on each single update.
		for(const auto& [name, val]: rcCs->channels)
		{
			if(name == "Master")
				handler.onVolume(Denon::Channel::Master, val.volume);
			else
				handler.onZoneVolume(name, val.volume);
		}
	}
}


} // namespace Upnp
} // namespace Denon
