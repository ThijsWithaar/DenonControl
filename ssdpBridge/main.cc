#include <Denon/ssdp.h>

#include <algorithm>
#include <iostream>
#include <fstream>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>


namespace boost {
namespace serialization {


template<class Archive>
void serialize(Archive & ar, Ssdp::Interface& s, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(s.name) &
		BOOST_SERIALIZATION_NVP(s.ip) &
		BOOST_SERIALIZATION_NVP(s.netmask);
}


template<class Archive>
void serialize(Archive & ar, Ssdp::Bridge::Settings & s, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(s.server) & BOOST_SERIALIZATION_NVP(s.client);
}


} // namespace serialization
} // namespace boost


bool operator==(Ssdp::Interface a, Ssdp::Interface b)
{
	return a.name == b.name;
}


Ssdp::Interface AskInterface(std::vector<Ssdp::Interface> itfs)
{
	for(size_t n=0; n < itfs.size(); n++)
	{
		auto itf = itfs[n];
		std::cout << boost::format("%2i: %-10s, ip %-10s / %-10s\n") % n % itf.name % itf.ip % itf.netmask;
	}
	std::cout << "Enter index : ";

	size_t idx;
	std::cin >> idx;
	std::cout << std::endl;

	return itfs[idx];
}


void Setup()
{
	Ssdp::Bridge::Settings settings;

	auto itfs = Ssdp::GetNetworkInterfaces();

	std::cout << "Select server interface\n";
	settings.server = AskInterface(itfs);

	itfs.erase(std::remove_if(itfs.begin(), itfs.end(), [&](auto it)
	{
		return it == settings.server;
	}), itfs.end());

	std::cout << "Select client interface\n";
	settings.client = AskInterface(itfs);

	std::string cfgName = Ssdp::GetConfigurationPath("ssdpBridge", true);
	std::ofstream ofs(cfgName);
	boost::archive::xml_oarchive oa(ofs);
	oa << BOOST_SERIALIZATION_NVP(settings);
	std::cout << "Configuration saved to : " << cfgName << "\n";
}


void Bridge()
{
	Ssdp::Bridge::Settings settings;
	std::string cfgName = Ssdp::GetConfigurationPath("ssdpBridge", false);
	std::ifstream fs(cfgName);
	boost::archive::xml_iarchive ar(fs);
	ar >> BOOST_SERIALIZATION_NVP(settings);
	std::cout << "Configuration loaded from : " << cfgName << "\n";

	boost::asio::io_context io_context;
	Ssdp::Bridge bridge(io_context, settings);
	io_context.run();
}


int main(int argc, char* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");

	bool help, setup, bridge;

	desc.add_options()
		("help", po::bool_switch(&help), "show the help message")
		("setup", po::bool_switch(&setup), "interactive setup")
		("bridge", po::bool_switch(&bridge), "run the bridge")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (help)
		std::cout << desc << "\n";

	if(setup)
		Setup();

	if(bridge)
		Bridge();
}
