#include <algorithm>
#include <iostream>
#include <fstream>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

#ifdef __linux__
	#include <sys/types.h>
	#include <grp.h>
	#include <pwd.h>
#endif

#include <Denon/network/ssdp.h>
#include <Denon/network/ssdpBridge.h>


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
void serialize(Archive & ar, MiniSsdp::Server::Settings& s, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(s.socketName);
}


template<class Archive>
void serialize(Archive & ar, Ssdp::Bridge::Settings & s, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(s.server) &
	     BOOST_SERIALIZATION_NVP(s.client) &
	     BOOST_SERIALIZATION_NVP(s.ipc);
}


} // namespace serialization
} // namespace boost


bool operator==(Ssdp::Interface a, Ssdp::Interface b)
{
	return a.name == b.name;
}


std::ostream& operator<<(std::ostream& os, std::vector<Ssdp::Interface> itfs)
{
	for(auto itf: itfs)
		os << itf.name << " ";
	return os;
}


class SettingsConfiguration
{
public:
	SettingsConfiguration()
	{
		m_cfgName = Ssdp::GetConfigurationPath("ssdpBridge", true);
		try
		{
			std::ifstream fs(m_cfgName);
			boost::archive::xml_iarchive ar(fs);
			ar >> BOOST_SERIALIZATION_NVP(settings);
		}
		catch(...)
		{
			std::cerr << "Could not load settings " << m_cfgName << "\n";
		}
	}

	~SettingsConfiguration()
	{
		std::cout << "Saving " << m_cfgName << "\n";
		std::ofstream fs(m_cfgName);
		boost::archive::xml_oarchive ar(fs);
		ar << BOOST_SERIALIZATION_NVP(settings);
	}

	Ssdp::Bridge::Settings settings;
private:
	std::string m_cfgName;
};


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
	SettingsConfiguration cfg;

	auto itfs = Ssdp::GetNetworkInterfaces();

	std::cout << "Select server interface\n";
	cfg.settings.server = AskInterface(itfs);

	itfs.erase(std::remove_if(itfs.begin(), itfs.end(), [&](auto it)
	{
		return it == cfg.settings.server;
	}), itfs.end());

	std::cout << "Select client interface\n";
	cfg.settings.client = AskInterface(itfs);
}


void DropPriviliges(const char* username)
{
#ifdef __linux__
	struct passwd *pw = NULL;
	pw = getpwnam(username);
	const char* name = nullptr;
	int gid = 0;
	initgroups(pw->pw_name, pw->pw_gid);
	setgid(pw->pw_gid);
	setuid(pw->pw_uid);
#endif
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

	// DropPriviliges(username);

	io_context.run();
}


int main(int argc, char* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");

	bool help, setup, bridge;
	std::string server, client, ipc;

	desc.add_options()
		("help", po::bool_switch(&help), "show the help message")
		("setup", po::bool_switch(&setup), "interactive setup")
		("bridge", po::bool_switch(&bridge), "run the bridge")
		("server", po::value<std::string>(&server), "set server interface name")
		("client", po::value<std::string>(&client), "set client interface name")
		("ipc", po::value<std::string>(&ipc), "set minissdp socket filename")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (help || argc == 1)
		std::cout << desc << "\n";

	if(!server.empty())
	{
		auto itfs = Ssdp::GetNetworkInterfaces();
		auto itf = std::find_if(itfs.begin(), itfs.end(), [&server](Ssdp::Interface itf)
		{
			return itf.name == server;
		});

		SettingsConfiguration cfg;
		cfg.settings.server = *itf;
	}

	if(!client.empty())
	{
		auto itfs = Ssdp::GetNetworkInterfaces();
		auto itf = std::find_if(itfs.begin(), itfs.end(), [&client](Ssdp::Interface itf)
		{
			return itf.name == client;
		});
		SettingsConfiguration cfg;
		if(itf == itfs.end())
			std::cerr << "client " << client << " not found in " << itfs << "\n";
		else
			cfg.settings.client = *itf;
	}

	if(!ipc.empty())
	{
		SettingsConfiguration cfg;
		cfg.settings.ipc.socketName = ipc;
	}

	if(setup)
		Setup();

	if(bridge)
		Bridge();
}
