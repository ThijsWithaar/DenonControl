#include <Denon/appInterface.h>

#include <iostream>

#include <boost/property_tree/xml_parser.hpp>


namespace Denon {


enum class Query {
	AppCommand, AppCommand300, DeviceInfo, NetAudioStatus
};


std::map<Query, std::string> g_QueryUrls = {
	{Query::AppCommand, "/goform/AppCommand.xml"},
	{Query::AppCommand300, "/goform/AppCommand0300.xml"},
	{Query::DeviceInfo, "/goform/Deviceinfo.xml"},
	{Query::NetAudioStatus, "/goform/formNetAudio_StatusXml.xml"},
	// From wireshark capture of Denon android app:
	//{Query::?, "goform/formiPhoneAppControlJudge.xml"},
	//{"/goform/AppCommand0300.xml"}		// id : 3, name: GetSoundMode

};


void Parse(ZoneCapabilities& r, const boost::property_tree::ptree& pt)
{
}


void Parse(DeviceCapabilities& r, const boost::property_tree::ptree& pt)
{
	auto setup = pt.get_child("Setup");
}


//-- AppInterface --


AppInterface::AppInterface(std::unique_ptr<Http::BlockingConnection>&& con):
	m_http(std::move(con))
{
}


const Http::Response& AppInterface::Get(std::string path)
{
	return m_http->Http({
		Http::Method::Get,
		path,
		{},
		""
	});
}


const Http::Response& AppInterface::Post(std::string path, std::string body)
{
	return m_http->Http({
		Http::Method::Post,
		path,
		{},
		body
	});
}


boost::property_tree::ptree AppInterface::Parse(const Http::Response& r)
{
	std::stringstream ss;
	ss << r.body;
	boost::property_tree::ptree pt;
	boost::property_tree::read_xml(ss, pt);
	return pt;
}


DeviceInfo AppInterface::GetDeviceInfo()
{
	DeviceInfo r;
	auto response = Get(g_QueryUrls.at(Query::DeviceInfo));
	auto pt = Parse(response);

	auto di = pt.get_child("Device_Info");

	r.version = di.get<int>("DeviceInfoVers");
	r.commApiVersion = di.get<int>("CommApiVers");
	r.catName = di.get<std::string>("CategoryName");
	r.modelName = di.get<std::string>("ModelName");
	r.manualModelName = di.get<std::string>("ManualModelName");
	r.mac = di.get<std::string>("MacAddress");
	Denon::Parse(r.capabilities, di.get_child("DeviceCapabilities"));

	//std::cout << " .version = " << r.version << "\n";
	std::cout << "modelName = " << r.modelName << "\n";
	for(auto& leaf: di)
	{
		//std::cout << leaf.first << "\n";
		if(leaf.first == "DeviceZoneCapabilities")
		{
			ZoneCapabilities zc;
			Denon::Parse(zc, leaf.second);
			r.zoneCapabilities.push_back(zc);
		}
	}

	return r;
}


void AppInterface::Send(const std::string& cmd)
{
	auto response = Get("/goform/formiPhoneAppDirect.xml?" + cmd);
	#if 1
		std::cout << "HttpDevice::Send:\n" << response.body << "\n";
	#endif
}


DeviceStatus AppInterface::GetDeviceStatus()
{
	DeviceStatus ds;

	auto response = AppCommand({
		// All id 1:
		"GetAllZonePowerStatus",
		"GetAllZoneSource",
		"GetAllZoneStereo",
		"GetAllZoneVolume",
		"GetAllZoneMuteStatus",
		"GetSurroundModeStatus"
	// Others, from wireshark capture
		//"GetECOMeter",
		//"GetAutoStandby",
		//"GetSpABStatus"
		//id: 3, name: "GetSetupLock"
	});
	/*
	<cmd id="3">
	<name>GetActiveSpeaker</name>
	<list>
		<param name="activespall"></param>
	</list>
	</cmd>
	*/

	boost::property_tree::ptree pt = Parse(response);
	auto rx = pt.get_child("rx");
	auto it = rx.begin();

	// Power
	boost::property_tree::ptree power = (it++)->second;
	for(auto& zone: power)
		ds.zones[zone.first].power = zone.second.get_value<std::string>();

	// Source
	boost::property_tree::ptree source = (it++)->second;
	for(auto& zone: source)
	{
		auto source = zone.second.get_child("source");
		ds.zones[zone.first].source = source.get_value<std::string>();
	}

	// Volume
	boost::property_tree::ptree volume = (it++)->second;
	for(auto& zone: volume)
	{
		auto vol = zone.second.get_child("volume");
		ds.zones[zone.first].volume = vol.get_value<double>();
	}

	// Mute
	boost::property_tree::ptree mute = (it++)->second;
	for(auto& zone: mute)
	{
		ds.zones[zone.first].mute = zone.second.get_value<std::string>();
	}

	// Surround
	boost::property_tree::ptree surround = (it++)->second;
	ds.surround = surround.get_child("surround").get_value<std::string>();

	return ds;
}



/**
POST /goform/AppCommand0300.xml HTTP/1.1
Content-Type: text/xml; charset="utf-8"
Content-Length: 152
HOST: 192.168.4.7:8080
User-Agent: CyberGarage-HTTP/1.1 DLNADOC/1.50

<?xml version="1.0" encoding="utf-8"?>
<tx>
 <cmd id="3">
  <name>SetSoundMode</name>
  <list>
   <param name="genre">4</param>
  </list>
 </cmd>
</tx>
*/
void AppInterface::SetSoundMode(int mode)
{
	boost::property_tree::ptree param("param");
	param.put("<xmlattr>.name", "genre");
	param.put_value(mode);

	boost::property_tree::ptree list("list");
	list.put_child("param", param);

	boost::property_tree::ptree name("name");
	name.put_value("SetSoundMode");

	boost::property_tree::ptree cmd("cmd");
	cmd.put("<xmlattr>.id", 3);
	cmd.put_child("name", name);
	cmd.put_child("list", list);

	AppCommand3({cmd});
}


const Http::Response& AppInterface::AppCommand3(std::vector<boost::property_tree::ptree> commands)
{
	boost::property_tree::ptree pt, tx;
	for(auto& cmd: commands)
		tx.put_child("cmd", cmd);
	pt.put_child("tx", tx);

	std::stringstream bodySend;
	boost::property_tree::write_xml(bodySend, pt);
	return Post(g_QueryUrls.at(Query::AppCommand300), bodySend.str());
}


const Http::Response& AppInterface::AppCommand(std::vector<std::string> attributes)
{
	auto url = g_QueryUrls.at(Query::AppCommand);
	boost::property_tree::ptree pt, tx;

	int id = 1;
	for(auto& att: attributes)
	{
		boost::property_tree::ptree attr;
		attr.put("<xmlattr>.id", id++);
		attr.put_value(att);
		tx.push_back({"cmd", attr});
	}
	pt.put_child("tx", tx);

	std::stringstream bodySend;
	boost::property_tree::write_xml(bodySend, pt);
	//std::cout << "AppCommand Post body:\n" << bodySend.str() << "\n";

	auto& response = Post(url, bodySend.str());
#if 0
	std::string body { asio::buffers_begin(response.body().data()),
                   asio::buffers_end(response.body().data()) };
	std::cout << "HttpDevice::AppCommand:\n" << body << "\n";
#endif
	return response;
}


} // namespace Denon
