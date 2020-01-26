#include <Denon/upnpControl.h>

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Denon/string.h>


namespace Denon {
namespace Upnp {


boost::property_tree::ptree ParseXml(std::string_view resp)
{
	std::stringstream ss;
	ss << resp;
	boost::property_tree::ptree pt;
	boost::property_tree::read_xml(ss, pt);
	return pt;
}


boost::property_tree::ptree DecodeXmlResponse(const Denon::Http::Response& resp, std::string param)
{
	auto pt = ParseXml(resp.body);
	auto acr = pt.get<std::string>("s:Envelope.s:Body." + param);
	//std::cout << xmlDecode(acr) << "\n";
	return ParseXml(acr);
}


//-- SoapRequest --


SoapRequest::operator Denon::Http::Request()
{
	Denon::Http::Request r;
	r.path = path;
	r.method = Denon::Http::Method::Get;
	r.fields["Content-Type"] = "text/xml";
	r.fields["SOAPACTION"] = '"' + type + '#' + actionName + '"';

	boost::property_tree::ptree action;
	action.put("<xmlattr>.xmlns:u", type);
	for(auto& prm: parameters)
	{
		boost::property_tree::ptree p;
		p.put_value(prm.second);
		action.add_child(prm.first, p);
	}

	boost::property_tree::ptree body;
	body.put_child("u:" + actionName, action);

	boost::property_tree::ptree envelope;
	envelope.put("<xmlattr>.s:encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
	envelope.put("<xmlattr>.xmlns:s", "http://schemas.xmlsoap.org/soap/envelope/");
	envelope.put_child("s:Body", body);

	boost::property_tree::ptree pt;
	pt.put_child("s:Envelope", envelope);
	std::stringstream bodySend;
	boost::property_tree::write_xml(bodySend, pt);
	r.body = bodySend.str();
	return r;
}


//-- AV Transport --


AvTransport::AvTransport(Denon::Http::BlockingConnection* con):
	m_con(con)
{
	m_request.type = "urn:schemas-upnp-org:service:AVTransport:1";
	m_request.path = "/upnp/control/renderer_dvc/AVTransport";
}


AvTransport::Metadata ParseTrackMetadata(std::string_view xml)
{
	auto pt = ParseXml(xml);
	auto item = pt.get_child("DIDL-Lite.item");
	AvTransport::Metadata r;
	r.title = item.get<std::string>("dc:title");
	r.creator = item.get<std::string>("dc:creator");

	r.album = item.get<std::string>("upnp:album");

	return r;
}


// Expects 'Event.InstanceID' as root in `iid`
AvTransport::CurrentState ParseAvTransportState(const boost::property_tree::ptree& iid)
{
	AvTransport::CurrentState r;
	r.transportURI = iid.get<std::string>("AVTransportURI.<xmlattr>.val");
	r.currentMediaDuration = iid.get<std::string>("CurrentMediaDuration.<xmlattr>.val");
	r.currentTrackURI = iid.get<std::string>("CurrentTrackURI.<xmlattr>.val");
	r.transportState = iid.get<std::string>("TransportState.<xmlattr>.val");

	auto curMd = iid.get<std::string>("CurrentTrackMetaData.<xmlattr>.val");
	r.currentTrackMetaData = ParseTrackMetadata(curMd);
	//std::cout << curMd << "\n";
	return r;
}


std::ostream& operator<<(std::ostream& os, const AvTransport::Metadata& md)
{
	os << "Title   : " << md.title << "\n";
	os << "Creator : " << md.creator << "\n";
	os << "Album   : " << md.album << "\n";
	return os;
}


std::ostream& operator<<(std::ostream& os, const AvTransport::CurrentState& cs)
{
	os << "transportURI         : " << cs.transportURI << "\n";
	os << "currentMediaDuration : " << cs.currentMediaDuration << "\n";
	os << "currentTrackURI      : " << cs.currentTrackURI << "\n";
	os << "transportState       : " << cs.transportState << "\n";
	os << cs.currentTrackMetaData << "\n";
	return os;
}


AvTransport::CurrentState AvTransport::GetCurrentState()
{
	m_request.actionName = "GetCurrentState";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetCurrentStateResponse.CurrentState");
	auto& iid = pt.get_child("Event.InstanceID");
	return ParseAvTransportState(iid);
}


void AvTransport::GetCurrentTransportActions(int instanceId)
{
	m_request.actionName = "GetCurrentTransportActions";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	auto& resp = m_con->Http(m_request);
	//std::cout << "AvTransport::GetCurrentTransportActions: body\n" << resp.body << "\n";

	auto pt = ParseXml(resp.body).get_child("s:Envelope.s:Body.u:GetCurrentTransportActionsResponse");

	auto actions = pt.get<std::string>("Actions");
}


void AvTransport::GetDeviceCapabilities(int instanceId)
{
	m_request.actionName = "GetDeviceCapabilities";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetDeviceCapabilitiesResponse");
}


void AvTransport::GetMediaInfo(int instanceId)
{
	m_request.actionName = "GetMediaInfo";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetMediaInfoResponse");
}


AvTransport::PositionInfo AvTransport::GetPositionInfo(int instanceId)
{
	m_request.actionName = "GetPositionInfo";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	auto& resp = m_con->Http(m_request);
	auto pt = ParseXml(resp.body);
	auto& pi = pt.get_child("s:Envelope.s:Body.u:GetPositionInfoResponse");

	PositionInfo r;
	auto curMd = pi.get<std::string>("TrackMetaData");
	r.currentTrackMetaData = ParseTrackMetadata(curMd);
	r.relTime = pi.get<std::string>("RelTime");

	return r;
}


void AvTransport::GetTransportInfo(int instanceId)
{
	m_request.actionName = "GetTransportInfo";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	auto& resp = m_con->Http(m_request);
}


void AvTransport::GetTransportSettings(int instanceId)
{
	m_request.actionName = "GetTransportSettings";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	auto& resp = m_con->Http(m_request);
}


void AvTransport::Play(int instanceId)
{
	m_request.actionName = "Play";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
		{"Speed", "1"},
	};
	m_con->Http(m_request);
}


void AvTransport::Pause(int instanceId)
{
	m_request.actionName = "Pause";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	m_con->Http(m_request);
}


void AvTransport::Stop(int instanceId)
{
	m_request.actionName = "Stop";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	m_con->Http(m_request);
}


void AvTransport::Next(int instanceId)
{
	m_request.actionName = "Next";
	m_request.parameters = {
		{"InstanceID", std::to_string(instanceId)},
	};
	m_con->Http(m_request);
}


//-- Rendering Control --


std::ostream& operator<<(std::ostream& os, RenderingControl::Channel c)
{
	using Channel = RenderingControl::Channel;
	switch(c)
	{
	case Channel::Master: os << "Master"; break;
	case Channel::Zone1: os << "Zone1"; break;
	case Channel::Zone2: os << "Zone2"; break;
	case Channel::Zone3: os << "Zone3"; break;
	case Channel::Zone4: os << "Zone4"; break;
	}
	return os;
}


RenderingControl::RenderingControl(Denon::Http::BlockingConnection* con):
	m_con(con)
{
	m_request.type = "urn:schemas-upnp-org:service:RenderingControl:1";
	m_request.path = "/upnp/control/renderer_dvc/RenderingControl";
}


// Expects 'Event.InstanceID' as root in `iid`
RenderingControl::CurrentState ParseRenderingControlState(const boost::property_tree::ptree& iid)
{
	RenderingControl::CurrentState cs;
	for(const auto& it: iid)
	{
		if(it.first == "Mute")
		{
			auto channel = it.second.get<std::string>("<xmlattr>.channel");
			auto val = it.second.get<int>("<xmlattr>.val");
			cs.channels[channel].muted = val != 0;
		}
		else if(it.first == "Volume")
		{
			auto channel = it.second.get<std::string>("<xmlattr>.channel");
			cs.channels[channel].volume = it.second.get<int>("<xmlattr>.val");
		}
		else if(it.first == "VolumeDB")
		{
			auto channel = it.second.get<std::string>("<xmlattr>.channel");
			cs.channels[channel].volume = it.second.get<double>("<xmlattr>.val");
		}
		else if(it.first == "Bass")
			cs.bass = it.second.get<int>("<xmlattr>.val");
		else if(it.first == "Treble")
			cs.treble = it.second.get<int>("<xmlattr>.val");
		else if(it.first == "Subwoofer")
			cs.subwoofer = it.second.get<int>("<xmlattr>.val");
		else if(it.first == "Balance")
			cs.balance = it.second.get<int>("<xmlattr>.val");
		else if(it.first == "Preset")
			cs.preset = it.second.get<std::string>("<xmlattr>.val");
		else if(it.first == "PresetNameList")
		{
			auto pnl = it.second.get<std::string>("<xmlattr>.val");
			for(auto& name: split(pnl,","))
				cs.presetNames.push_back(std::string(name));
		}
	}
	return cs;
}


RenderingControl::CurrentState RenderingControl::GetCurrentState()
{
	m_request.actionName = "GetCurrentState";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetCurrentStateResponse.CurrentState");
	auto iid = pt.get_child("Event.InstanceID");

	auto cs  = ParseRenderingControlState(iid);
	return cs;
}


std::string RenderingControl::GetMute(Channel channel)
{
	m_request.actionName = "GetMute";
	m_request.parameters = {
		{"InstanceID", "0"},
		{"Channel", toString(channel)},
	};
	auto& resp = m_con->Http(m_request);
	return resp.body;
}


void RenderingControl::SetVolume(int instance, Channel channel, double volume)
{
	m_request.actionName = "GetMute";
	m_request.parameters = {
		{"InstanceID", std::to_string(instance)},
		{"Channel", toString(channel)},
		{"DesiredVolume", std::to_string(volume)}
	};
	m_con->Http(m_request);
}


//-- DenonAct --


DenonAct::DenonAct(Denon::Http::BlockingConnection* con):
	m_con(con)
{
	m_request.type = "urn:schemas-denon-com:service:ACT:1";
	m_request.path = "/ACT/control";
}


std::vector<DenonAct::ApInfo> DenonAct::GetAccessPointList()
{
	m_request.actionName = "GetAccessPointList";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetAccessPointListResponse.accessPointList");
	auto& ailist = pt.get_child("APInfoList");

	std::vector<ApInfo> r;
	for(auto& [key, ai]: ailist)
	{
		ApInfo i;
		i.ssid = ai.get<std::string>("SSID");
		i.protocol = ai.get<std::string>("Protocol");
		i.channel = ai.get<int>("Channel");
		i.signal = ai.get<int>("Signal");
		i.quality = ai.get<int>("Quality");
		i.security = ai.get<std::string>("SecurityMode");
		i.wps = ai.get<std::string>("WPS") != "NO";

		r.push_back(i);
	}

	return r;
}


DenonAct::AudioConfig ParseAudioConfig(const boost::property_tree::ptree& pt)
{
	auto& act = pt.get_child("AudioConfig");

	DenonAct::AudioConfig ac;
	ac.highpass = act.get<int>("highpass");
	ac.lowpass = act.get<int>("lowpass");
	ac.subwooferEnable = act.get<int>("subwooferEnable") != 0;
	ac.outputMode = act.get<std::string>("outputMode");
	ac.soundMode = act.get<std::string>("soundMode");
	ac.ampPower = act.get<int>("ampPower");
	auto asvs = act.get<std::string>("availableSoundModes");
	for(auto avsm: split(asvs))
		ac.availableSoundModes.push_back(std::string(avsm));
	ac.sourceDirect = act.get<int>("sourceDirect") != 0;
	ac.bassBoost = act.get<int>("bassBoost");

	return ac;
}


DenonAct::BluetoothConfig ParseBtConfig(const boost::property_tree::ptree& pt)
{
	auto btc = pt.get_child("BluetoothStatus");

	DenonAct::BluetoothConfig ret;
	ret.connectedStatus = btc.get<std::string>("connectedStatus");
	ret.connectionType = btc.get<std::string>("connectionType");
	ret.hasPairedDevices = btc.get<int>("hasPairedDevices") != 0;
	return ret;
}


DenonAct::AudioConfig DenonAct::GetAudioConfig()
{
	m_request.actionName = "GetAudioConfig";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetAudioConfigResponse.AudioConfig");
	return ParseAudioConfig(pt);
}


DenonAct::ZoneStatus ParseZoneStatus(const boost::property_tree::ptree& pt)
{
	auto azs = pt.get_child("AvrZoneStatus");

	DenonAct::ZoneStatus zs;
	for(auto zone: azs.get_child("Zones"))
	{
		DenonAct::Zone zn;
		zn.name = zone.second.get<std::string>("Name");
		zn.active = zone.second.get<int>("Active") != 0;
		zn.grouped = zone.second.get<int>("Grouped") != 0;
		zn.enabled = zone.second.get<int>("Enabled") != 0;
		if(auto ai = zone.second.get_optional<std::string>("AvailableInputs"))
			for(auto in: split(*ai, ","))
				zn.availableInputs.push_back(std::string(in));
		zs.zones.push_back(zn);
	}
	zs.minimised = azs.get<int>("Minimised") != 0;
	return zs;
}


std::vector<DenonAct::NetworkConfiguration> ParseNetworkConfigs(const boost::property_tree::ptree& pt)
{
	std::vector<DenonAct::NetworkConfiguration> r;
	for(auto& [key, ncfg]: pt.get_child("listNetworkConfigurations"))
	{
		DenonAct::NetworkConfiguration rs;
		rs.id = ncfg.get<int>("<xmlattr>.id") != 0;
		rs.dhcpOn = ncfg.get<int>("<xmlattr>.dhcpOn") != 0;
		rs.name = ncfg.get<std::string>("Name");
		rs.ip = ncfg.get<std::string>("IP");
		rs.netmask = ncfg.get<std::string>("Netmask");
		rs.gateway = ncfg.get<std::string>("Gateway");
		rs.gwMac = ncfg.get<std::string>("gwMac");

		if(auto wp = ncfg.get_child_optional("wirelessProfile"))
		{
			auto ssid = wp->get<std::string>("<xmlattr>.SSID");
			if(auto pass = wp->get_optional<std::string>("MODE.<xmlattr>.passPhrase"))
			{
			}
		}

		r.push_back(rs);
	}

	return r;
}


DenonAct::ZoneStatus DenonAct::GetAvrZoneStatus()
{
	m_request.actionName = "GetAvrZoneStatus";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetAvrZoneStatusResponse.status");
	return ParseZoneStatus(pt);
};


DenonAct::Speaker ParseSpeaker(boost::property_tree::ptree& pt)
{
	DenonAct::Speaker s;
	s.distance = pt.get<int>("distance");
	s.level = pt.get<int>("level");
	s.testTone = pt.get<int>("test_tone") != 0;
	return s;
}


std::ostream& operator<<(std::ostream& os, const DenonAct::Speaker& s)
{
	os << "distance: " << s.distance << ", level " << s.level;
	return os;
}

std::ostream& operator<<(std::ostream& os, const DenonAct::SpeakerConfig& sc)
{
	os << "front.left  = " << sc.front.left << "\n";
	os << "front.right = " << sc.front.right << "\n";
	//os << "center = " << sc.center << "\n";
	os << "rear.left   = " << sc.rear.left << "\n";
	os << "rear.right  = " << sc.rear.right << "\n";
	return os;
}


DenonAct::SpeakerConfig ParseSpeakerConfig(const boost::property_tree::ptree& pt)
{
	auto ssc = pt.get_child("SurroundSpeakerConfig");

	DenonAct::SpeakerConfig ret;
	ret.front.enabled = ssc.get<int>("Front.enabled") != 0;
	ret.front.left = ParseSpeaker(ssc.get_child("Front.Left"));
	ret.front.right = ParseSpeaker(ssc.get_child("Front.Right"));

	ret.center.enabled = ssc.get<int>("Center.enabled") != 0;
	ret.center.center = ParseSpeaker(ssc.get_child("Center.Center"));

	ret.subwoofer.enabled = ssc.get<int>("Subwoofer.enabled") != 0;
	ret.subwoofer.subwoofer = ParseSpeaker(ssc.get_child("Subwoofer.Subwoofer"));

	ret.rear.enabled = ssc.get<int>("Rear.enabled") != 0;
	ret.rear.left = ParseSpeaker(ssc.get_child("Rear.Left"));
	ret.rear.right = ParseSpeaker(ssc.get_child("Rear.Right"));

	ret.distanceUnit = ssc.get<std::string>("DistUnit");

	return ret;
}


DenonAct::SpeakerConfig DenonAct::GetSurroundSpeakerConfig()
{
	m_request.actionName = "GetSurroundSpeakerConfig";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetSurroundSpeakerConfigResponse.SurroundSpeakerConfig");

	return ParseSpeakerConfig(pt);
}


DenonAct::CurrentState DenonAct::GetCurrentState()
{
	m_request.actionName = "GetCurrentState";
	auto& resp = m_con->Http(m_request);
	auto cs = ParseXml(resp.body).get<std::string>("s:Envelope.s:Body.u:GetCurrentStateResponse.CurrentState");
	auto ev = ParseXml(cs).get_child("Event");

	CurrentState r;
	r.friendlyName = ev.get<std::string>("FriendlyName.<xmlattr>.val");
	r.heosNetId = ev.get<std::string>("HEOSNetId");

	auto ac = ev.get<std::string>("AudioConfig.<xmlattr>.val");
	r.audioConfig = ParseAudioConfig(ParseXml(ac));

	auto btc = ev.get<std::string>("BTConfig.<xmlattr>.val");
	r.btConfig = ParseBtConfig(ParseXml(btc));

	r.languageLocale = ev.get<std::string>("CurrentLanguageLocale");

	auto ncfgs = ev.get<std::string>("NetworkConfigurationList.<xmlattr>.val");
	r.networkConfigurations = ParseNetworkConfigs(ParseXml(ncfgs));

	auto spc = ev.get<std::string>("SurroundSpeakerConfig.<xmlattr>.val");
	r.speakerConfig = ParseSpeakerConfig(ParseXml(spc));

	r.wifiApSsid = ev.get<std::string>("WifiApSsid");
	r.wirelessState = ev.get<std::string>("WirelessState");

	auto zs = ev.get<std::string>("AvrZoneStatus.<xmlattr>.val");
	r.zoneStatus = ParseZoneStatus(ParseXml(zs));

	return r;
}


std::ostream& operator<<(std::ostream& os, DenonAct::BluetoothAction action)
{
	using BluetoothAction = DenonAct::BluetoothAction;
	switch(action)
	{
		case BluetoothAction::NONE: os << "NONE"; break;
		case BluetoothAction::START_PAIRING: os << "START_PAIRING"; break;
		case BluetoothAction::CANCEL_PAIRING: os << "CANCEL_PAIRING"; break;
		case BluetoothAction::CONNECT: os << "CONNECT"; break;
		case BluetoothAction::DISCONNECT: os << "DISCONNECT"; break;
		case BluetoothAction::CLEAR_PAIRED_LIST: os << "CLEAR_PAIRED_LIST"; break;
	}
	return os;
}


void DenonAct::SetBluetoothAction(int index, BluetoothAction action)
{
	m_request.actionName = "SetBluetoothAction";
	m_request.parameters = {
		{"BTAction", toString(action)},
		{"BTIndex", std::to_string(index)},
	};
	auto& resp = m_con->Http(m_request);
}


} // namespace Upnp
} // namespace Denon
