#include <Denon/network/scdp.h>

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>


namespace Denon {


void ParseService(SCDP::Service& dst, const boost::property_tree::ptree& pt)
{
	dst.type = pt.get<std::string>("serviceType");
	dst.id = pt.get<std::string>("serviceId");
	dst.scdpUrl = pt.get<std::string>("SCPDURL");
	dst.controlUrl = pt.get<std::string>("controlURL");
	dst.eventUrl = pt.get<std::string>("eventSubURL");
}


void ParseDevice(SCDP::Device& dst, const boost::property_tree::ptree& pt)
{
	dst.type = pt.get<std::string>("deviceType");
	dst.friendlyname = pt.get<std::string>("friendlyName");
	dst.udn = pt.get<std::string>("UDN");

	if(auto serviceList = pt.get_child_optional("serviceList"))
	{
		for(auto& service: *serviceList)
		{
			dst.services.push_back({});
			ParseService(dst.services.back(), service.second);
		}
	}
}


SCDP::SCDP(std::string_view xml)
{
	std::stringstream ssxml;
	ssxml << xml;
	boost::property_tree::ptree pt;
	boost::property_tree::read_xml(ssxml, pt);

	auto rootdev = pt.get_child("root.device");
	ParseDevice(root, rootdev);
	audyssey.version = rootdev.get<int>("DMH:X_Audyssey");
	audyssey.port = rootdev.get<int>("DMH:X_AudysseyPort");
	audyssey.webApiPort = rootdev.get<int>("DMH:X_WebAPIPort");

	auto devlist = rootdev.get_child("deviceList");
	for(auto& dev: devlist)
	{
		devices.push_back({});
		ParseDevice(devices.back(), dev.second);
	}
}


} // namespace Denon
