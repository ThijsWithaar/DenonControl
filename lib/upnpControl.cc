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
	auto ud = urlDecode(acr);
	std::cout << ud << "\n";
	return ParseXml(ud);
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


//-- Rendering Control --


RenderingControl::RenderingControl(Denon::Http::BlockingConnection* con):
	m_con(con)
{
	m_request.type = "urn:schemas-upnp-org:service:RenderingControl:1";
	m_request.path = "/upnp/control/renderer_dvc/RenderingControl";
}


/**
 <Event xmlns="urn:schemas-upnp-org:metadata-1-0/RCS/">
  <InstanceID val="0">
    <Mute channel="Master" val="0"/>
    <Mute channel="Zone1" val="0"/>
    <Mute channel="Zone2" val="0"/>
    <Mute channel="Zone3" val="0"/>
    <Mute channel="Zone4" val="0"/>
    <PresetNameList val="FactoryDefaults,InstallationDefaults,Disabled,Accoustic,Classical,Jazz,Pop,Rock"/>
    <Volume channel="Master" val="44"/>
    <VolumeDB channel="Master" val="44"/>
    <Volume channel="Zone1" val="21"/>
    <VolumeDB channel="Zone1" val="21"/>
    <Volume channel="Zone2" val="44"/>
    <VolumeDB channel="Zone2" val="44"/>
    <Volume channel="Zone3" val="0"/>
    <VolumeDB channel="Zone3" val="0"/>
    <Volume channel="Zone4" val="0"/>
    <VolumeDB channel="Zone4" val="0"/>
    <SetVolumeId channel="Master" val=""/>
    <SetMuteId channel="Master" val=""/>
    <Preset val="InstallationDefaults"/>
    <Bass val="5"/>
    <Treble val="5"/>
    <Subwoofer val="15"/>
    <Balance val="50"/>
  </InstanceID>
</Event>
*/
RenderingControl::CurrentState RenderingControl::GetCurrentState()
{
	m_request.actionName = "GetCurrentState";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetCurrentStateResponse.CurrentState");
	auto iid = pt.get_child("Event.InstanceID");

	CurrentState cs;
	for(auto& it: iid)
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
	}

	return cs;
}


std::string RenderingControl::GetMute(Channel channel)
{
	m_request.actionName = "GetMute";
	m_request.parameters = {
		{"InstanceID", "0"},
		{"Channel", "Master"},
	};
	auto& resp = m_con->Http(m_request);
	return resp.body;
}


//-- DenonAct --


DenonAct::DenonAct(Denon::Http::BlockingConnection* con):
	m_con(con)
{
	m_request.type = "urn:schemas-denon-com:service:ACT:1";
	m_request.path = "/ACT/control";
}


DenonAct::AudioConfig DenonAct::GetAudioConfig()
{
	m_request.actionName = "GetAudioConfig";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetAudioConfigResponse.AudioConfig");
	auto act = pt.get_child("AudioConfig");

	AudioConfig ac;
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


/**
<?xml version="1.0"?>
	<AvrZoneStatus>
	<Zones>
		<Zone1>
		<Name>MAIN ZONE</Name>
		<Active>0</Active>
		<Grouped>0</Grouped>
		<Enabled>1</Enabled>
		</Zone1>
		<Zone2>
		<Name>ZONE2</Name>
		<Active>0</Active>
		<Grouped>0</Grouped>
		<Enabled>1</Enabled>
		<AvailableInputs>TUNER,PHONO</AvailableInputs>
		</Zone2>
	</Zones>
	<Minimised>0</Minimised>
	<GroupName/>
	</AvrZoneStatus>
*/
DenonAct::ZoneStatus DenonAct::GetAvrZoneStatus()
{
	m_request.actionName = "GetAvrZoneStatus";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetAvrZoneStatusResponse.status");
	auto azs = pt.get_child("AvrZoneStatus");

	ZoneStatus zs;
	for(auto zone: azs.get_child("Zones"))
	{
		Zone zn;
		zn.name = zone.second.get<std::string>("Name");
		zn.active = zone.second.get<int>("Active") != 0;
		zn.grouped = zone.second.get<int>("Grouped") != 0;
		zn.enabled = zone.second.get<int>("Enabled") != 0;

		zs.zones.push_back(zn);
	}

	return zs;
};


DenonAct::Speaker ParseSpeaker(boost::property_tree::ptree& pt)
{
	DenonAct::Speaker s;
	s.distance = pt.get<int>("distance");
	s.level = pt.get<int>("level");
	s.testTone = pt.get<int>("test_tone") != 0;
	return s;
}


DenonAct::SpeakerConfig DenonAct::GetSurroundSpeakerConfig()
{
	m_request.actionName = "GetSurroundSpeakerConfig";
	auto& resp = m_con->Http(m_request);
	auto pt = DecodeXmlResponse(resp, "u:GetSurroundSpeakerConfigResponse.SurroundSpeakerConfig");
	auto ssc = pt.get_child("SurroundSpeakerConfig");

	SpeakerConfig ret;
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


} // namespace Upnp
} // namespace Denon
