#pragma once

#include <map>
#include <string>
#include <vector>

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
	};

	RenderingControl(Denon::Http::BlockingConnection* con);

	CurrentState GetCurrentState();

	std::string GetMute(Channel channel);

private:
	Denon::Http::BlockingConnection* m_con;
	Denon::Upnp::SoapRequest m_request;
};

/// Description in http://192.168.4.7:60006/ACT/SCPD.xml
class DenonAct
{
public:
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

	struct Zone
	{
		std::string name;
		bool active;
		bool grouped;
		bool enabled;
	};

	struct ZoneStatus
	{
		std::vector<Zone> zones;
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

	DenonAct(Denon::Http::BlockingConnection* con);

	AudioConfig GetAudioConfig();

	ZoneStatus GetAvrZoneStatus();

	SpeakerConfig GetSurroundSpeakerConfig();

private:
	Denon::Http::BlockingConnection* m_con;
	Denon::Upnp::SoapRequest m_request;
};


} // namespace Upnp
} // namespace Denon
