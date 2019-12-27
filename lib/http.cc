#include <Denon/http.h>

#include <iostream>
#include <fstream>


#include <boost/beast/version.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>


namespace Denon {


/// Parse a beast reply into a property_tree, does copy data a few times
boost::property_tree::ptree ParseXml(const boost::beast::multi_buffer::const_buffers_type& buf)
{
	boost::property_tree::ptree pt;
	std::string str{asio::buffers_begin(buf), asio::buffers_end(buf)};
	std::stringstream bodyss;
	bodyss << str;
	boost::property_tree::read_xml(bodyss, pt);
	return pt;
}


enum class Query {
	AppCommand, DeviceInfo, NetAudioStatus
};


std::map<Query, std::string> g_QueryUrls = {
	{Query::AppCommand, "/goform/AppCommand.xml"},
	{Query::DeviceInfo, "/goform/Deviceinfo.xml"},
	{Query::NetAudioStatus, "/goform/formNetAudio_StatusXml.xml"},
};


HttpDevice::HttpDevice(std::string host, int port):
	m_socket(m_ioc),
	m_host(host)
{
	tcp::resolver resolver(m_ioc);
	auto const results = resolver.resolve(host, std::to_string(port));
	boost::asio::connect(m_socket, results.begin(), results.end());
}


void Parse(ZoneCapabilities& r, const boost::property_tree::ptree& pt)
{
}


void Parse(DeviceCapabilities& r, const boost::property_tree::ptree& pt)
{
	auto setup = pt.get_child("Setup");
}


DeviceInfo HttpDevice::GetDeviceInfo()
{
	DeviceInfo r;
	auto response = Get(g_QueryUrls.at(Query::DeviceInfo));

	/*{
	   std::string body { asio::buffers_begin(response.body().data()),
                   asio::buffers_end(response.body().data()) };
		std::ofstream of("http.body3");
		of << body;
	}*/
	boost::property_tree::ptree pt = ParseXml(response.body().data());
	auto di = pt.get_child("Device_Info");

	r.version = di.get<int>("DeviceInfoVers");
	r.commApiVersion = di.get<int>("CommApiVers");
	r.catName = di.get<std::string>("CategoryName");
	r.modelName = di.get<std::string>("ModelName");
	r.manualModelName = di.get<std::string>("ManualModelName");
	r.mac = di.get<std::string>("MacAddress");
	Parse(r.capabilities, di.get_child("DeviceCapabilities"));

	//std::cout << " .version = " << r.version << "\n";
	std::cout << "modelName = " << r.modelName << "\n";
	for(auto& leaf: di)
	{
		//std::cout << leaf.first << "\n";
		if(leaf.first == "DeviceZoneCapabilities")
		{
			ZoneCapabilities zc;
			Parse(zc, leaf.second);
			r.zoneCapabilities.push_back(zc);
		}
	}

	return r;
}


void HttpDevice::Send(const std::string& cmd)
{
	auto response = Get("/goform/formiPhoneAppDirect.xml?" + cmd);
	#if 1
		std::string body { asio::buffers_begin(response.body().data()),
					asio::buffers_end(response.body().data()) };
		std::cout << "HttpDevice::Send:\n" << body << "\n";
	#endif
}


DeviceStatus HttpDevice::GetDeviceStatus()
{
	DeviceStatus ds;

	auto response = AppCommand({
		"GetAllZonePowerStatus",
		"GetAllZoneSource",
		"GetAllZoneVolume",
		"GetAllZoneMuteStatus",
		"GetSurroundModeStatus"
	});

	boost::property_tree::ptree pt = ParseXml(response.body().data());
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


http::response<http::dynamic_body> HttpDevice::AppCommand(std::vector<std::string> attributes)
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

	auto response = Post(url, bodySend.str());
#if 0
	std::string body { asio::buffers_begin(response.body().data()),
                   asio::buffers_end(response.body().data()) };
	std::cout << "HttpDevice::AppCommand:\n" << body << "\n";
#endif
	return response;
}


http::response<http::dynamic_body> HttpDevice::Get(std::string path)
{
	http::request<http::string_body> req{http::verb::get, path, 11};
	req.set(http::field::host, m_host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.prepare_payload();

	http::write(m_socket, req);

	http::response<http::dynamic_body> res;
	http::read(m_socket, m_buffer, res);
	return res;
}


http::response<http::dynamic_body> HttpDevice::Post(std::string path, std::string body)
{
	http::request<http::string_body> req{http::verb::post, path, 11};
	req.set(http::field::host, m_host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.body() = body;
	req.prepare_payload();

	http::write(m_socket, req);

	http::response<http::dynamic_body> res;
	http::read(m_socket, m_buffer, res);
	return res;
}


} // namespace Denon
