#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include <Denon/http.h>


namespace Denon {
namespace Upnp {


/// Define a SOAP request, as HTPP request
class SoapRequest
{
public:
	std::string type;
	std::string actionName;
	std::string path;
	std::map<std::string, std::string> parameters;

	operator Denon::Http::Request();
};

/// As defined by http://192.168.4.7:60006/upnp/scpd/renderer_dvc/RenderingControl.xml
class RenderingControl
{
public:
	enum class Channel
	{
		Master, Zone1, Zone2, Zone3, Zone4
	};

	struct ChannelState
	{
		bool muted;
		double volume, volumeDb;
	};

	struct CurrentState
	{
		std::map<std::string, ChannelState> channels;
		double bass, treble, subwoofer, balance;
		std::string preset;
		std::vector<std::string> presetNames;
	};

	RenderingControl(Denon::Http::BlockingConnection* con);

	CurrentState GetCurrentState();

	std::string GetMute(Channel channel);

	void SetVolume(int instance, Channel channel, double volume);

private:
	Denon::Http::BlockingConnection* m_con;
	Denon::Upnp::SoapRequest m_request;
};

/// Description in http://192.168.4.7:60006/ACT/SCPD.xml
class DenonAct
{
public:
	struct ApInfo
	{
		std::string ssid, protocol, security;
		int channel, signal, quality;
		bool wps;
	};

	struct AudioConfig
	{
		int highpass, lowpass;
		bool subwooferEnable;
		std::string outputMode;
		std::string soundMode;
		std::vector<std::string> availableSoundModes;
		int ampPower;
		bool sourceDirect;
		int bassBoost;
	};

	struct BluetoothConfig
	{
		std::string connectedStatus;
		bool hasPairedDevices;
		std::string connectionType;
	};

	enum class BluetoothAction
	{
		NONE,START_PAIRING,CANCEL_PAIRING,CONNECT,DISCONNECT,CLEAR_PAIRED_LIST
	};

	struct NetworkConfiguration
	{
		int id;
		bool dhcpOn;
		std::string name, type, ip, netmask, gateway, gwMac;
	};

	struct Zone
	{
		std::string name;
		bool active;
		bool grouped;
		bool enabled;
		std::vector<std::string> availableInputs;
	};

	struct ZoneStatus
	{
		std::vector<Zone> zones;
		bool minimised;
	};

	struct Speaker
	{
		int distance, level;
		bool testTone;
	};

	struct SpeakerConfig
	{
		struct FrontRear
		{
			bool enabled;
			Speaker left, right;
		} front, rear;

		struct Center
		{
			bool enabled;
			Speaker center;
		} center;

		struct Subwoofer
		{
			bool enabled;
			Speaker subwoofer;
		} subwoofer;

		std::string distanceUnit;
	};

	struct CurrentState
	{
		std::string friendlyName, heosNetId;
		AudioConfig audioConfig;
		BluetoothConfig btConfig;
		std::string languageLocale;
		std::vector<NetworkConfiguration> networkConfigurations;
		SpeakerConfig speakerConfig;
		std::string wifiApSsid;
		std::string wirelessState;
		ZoneStatus zoneStatus;
	};

	DenonAct(Denon::Http::BlockingConnection* con);

	std::vector<ApInfo> GetAccessPointList();

	AudioConfig GetAudioConfig();

	ZoneStatus GetAvrZoneStatus();

	SpeakerConfig GetSurroundSpeakerConfig();

	CurrentState GetCurrentState();

	void SetBluetoothAction(int index, BluetoothAction action);

private:
	Denon::Http::BlockingConnection* m_con;
	Denon::Upnp::SoapRequest m_request;
};

boost::property_tree::ptree DecodeXmlResponse(const Denon::Http::Response& resp, std::string param);

DenonAct::AudioConfig ParseAudioConfig(const boost::property_tree::ptree& pt);

DenonAct::ZoneStatus ParseZoneStatus(const boost::property_tree::ptree& pt);

} // namespace Upnp
} // namespace Denon
