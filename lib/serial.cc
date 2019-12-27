#include <Denon/serial.h>

#include <iostream>
#include <map>
#include <sstream>


namespace Denon {


template<typename T>
std::string toString(T v)
{
	std::stringstream ss;
	ss << v;
	return ss.str();
};


bool startswith(std::string str, std::string start)
{
	return str.substr(0, start.size()).compare(start) == 0;
}


const std::map<std::string, Source> gSources = {
	{"TUNER", Source::TUNER},
	{"PHONO", Source::PHONO},
	{"CD", Source::CD},
	{"DVD", Source::DVD},
	{"MPLAY", Source::MediaPlayer},
	{"NET", Source::NET_USB},
};


const std::map<std::string, Surround> gSurround = {
	{"DIRECT", Surround::Direct},
	{"PURE DIRECT", Surround::PureDirect},
	{"STEREO", Surround::Stereo},
	{"STANDARD", Surround::Standard},
	{"DOLBY AUDIO-DD+DSUR", Surround::DolbyDigital},
};


const std::map<std::string, RoomEqualizer> gRoomEq = {
	{"AUDYSSEY", RoomEqualizer::Audyssey},
	{"BYP.LR", RoomEqualizer::Bypass},
	{"FLAT", RoomEqualizer::Flat},
	{"MANUAL", RoomEqualizer::Manual},
	{"OFF", RoomEqualizer::Off},
};


const std::map<std::string, DynamicVolume> gDynamicVolume = {
	{"OFF", DynamicVolume::Off},
	{"LIT", DynamicVolume::Light},
	{"MED", DynamicVolume::Medium},
	{"HEV", DynamicVolume::Heavy},
};


const std::map<std::string, Channel> gChannel = {
	{"MV", Channel::Master},
	{"CVFL ", Channel::FrontLeft},
	{"CVC ", Channel::Center},
};


const std::map<std::string, Speaker> gSpeaker = {
	{"FRO", Speaker::Front},
	{"CEN", Speaker::Center},
	{"SUA", Speaker::Sua},
	{"SWF", Speaker::SubWoofer},
};


const std::map<std::string, SpeakerType> gSpeakerType = {
	{"LAR", SpeakerType::Large},
	{"NON", SpeakerType::None},
	{"NO", SpeakerType::None},
	{"SMA", SpeakerType::Small},
	{"2SP", SpeakerType::TwoSpeaker},
};


template<typename Enum>
std::string lu(const std::map<std::string, Enum>& lut, Enum v)
{
	for(auto& it: lut)
		if(it.second == v)
			return it.first;
	return "";
}


std::ostream& operator<<(std::ostream& os, Source s)
{
	return os << lu(gSources, s);
}


std::ostream& operator<<(std::ostream& os, Surround s)
{
	return os << lu(gSurround, s);
}


void ParseResponse(std::string str, Response& response)
{
	if(str.size() < 2)
		return;

	std::string cmd = str.substr(0,2);
	std::string param = str.substr(2, str.size()-3);

	const std::map<std::string, std::function<void(void)>> handlers = {
		{"BT"   , [&]()
			{
				if(param == "TX ON")
					response.OnBluetooth(true);
				else if(param == "TX OFF")
					response.OnBluetooth(false);
			}},
		{"PWON" , [&](){ response.OnPower(true); }},
		{"PWOFF", [&](){ response.OnPower(false); }},
		{"MUON" , [&](){ response.OnMute(true); }},
		{"MUOFF", [&](){ response.OnMute(false); }},
		{"MV", [&]()
			{
				if(!startswith(str, "MVMAX"))
				{
					int vol = std::stoi(str.substr(2));
					if(vol > 100)
						vol /= 10;		// Denon seems to have implicit comma a start (0..1) values
					response.OnVolume(Channel::Master, vol);
				}
			}},
		{"MS", [&]()
			{
				//std::cout << "Denon surround '" << param << "'\n";
				if(gSurround.count(param))
					response.OnSurround(gSurround.at(param));
			}},
		{"SI", [&]()
			{
				if(gSources.count(param))
					response.OnInput(gSources.at(param));
			}},
		{"SS", [&]()
			{
				if(str == "SSINFAISFSV")
				{
					std::string sr = param.substr(10);
					if(sr == "441")
						response.OnSampleRate(44100);
					else if(sr == "48k")
						response.OnSampleRate(48000);
				}
				else if(startswith(param, "SPC"))
				{
					auto spkKey = param.substr(3,3);
					auto spkVal = param.substr(7);
					if(gSpeaker.count(spkKey) && gSpeakerType.count(spkVal))
					{
						response.OnSpeaker(gSpeaker.at(spkKey), gSpeakerType.at(spkVal));
					}
				}
			}},
		{"PS", [&]()
			{
				if(startswith(param, "CINEMA EQ."))
				{
					response.OnCinemaEq(param == "CINEMA EQ.ON");
				}
				if(startswith(param, "LFE"))
				{
					int val = std::stoi(param.substr(4));
					response.OnVolume(Channel::LFE, val);
				}
				else if(startswith(param, "BAS"))
				{
					int val = std::stoi(param.substr(4));
					response.OnVolume(Channel::Bass, val);
				}
				else if(startswith(param, "TRE"))
				{
					int val = std::stoi(param.substr(4));
					response.OnVolume(Channel::Treble, val);
				}
				else if(startswith(param, "ROOM"))
				{
					std::string eq = str.substr(10);
					//std::cout << "ROOM EQ : '" << eq << "'\n";
				}
				else if(startswith(param, "DYNEQ"))
				{
					response.OnDynamicEq(param == "DYNEQ ON");
				}
				else if(startswith(param, "DYNVOL"))
				{
					auto val = param.substr(7);
					//std::cout << "DYNVOL : '" << val << "'\n";
					if(gDynamicVolume.count(val))
						response.OnDynamicVolume(gDynamicVolume.at(val));
				}
				else if(startswith(param, "MULTEQ:"))
				{
					auto val = param.substr(7);
					if(gRoomEq.count(val))
						response.OnMultiEq(gRoomEq.at(val));
				}
			}},
	};


	for(auto& h: handlers)
	{
		if(startswith(str, h.first))
		{
			h.second();
			break;
		}
	}
}


//-- Command --


Command::Command(CommandConnection* conn):
	pConn(conn)
{
}


void Command::Power(bool p)
{
	pConn->Send(p ? "PWON\r" : "PWSTANDBY\r");
}


void Command::Mute(bool m)
{
	pConn->Send(m ? "MUON\r" : "MUOFF\r");
}


void Command::Volume(Channel c, int volume)
{
	pConn->Send(lu(gChannel,c) + std::to_string(volume)  + "\r");
}


void Command::Input(Source s)
{
	pConn->Send("SI" + lu(gSources, s) + "\r");
}


void Command::Surround(Denon::Surround s)
{
	pConn->Send("MS" + lu(gSurround, s) + "\r");
}


void Command::RoomEq(RoomEqualizer e)
{
	pConn->Send("PS ROOMEQ:" + lu(gRoomEq, e) + "\r");
}


void Command::DynamicEq(bool v)
{
	pConn->Send(v ? "PSDYNEQ ON\r" : "PSDYNEQ OFF\r");
}


void Command::DynamicVolume(Denon::DynamicVolume v)
{
	pConn->Send("PSDYNVOL " + lu(gDynamicVolume, v) + "\r");
}


void Command::CinemaEq(bool v)
{
	pConn->Send(v ? "PSCINEMA EQ.ON\r" : "PSCINEMA EQ.OFF\r");
}


void Command::MultiEq(RoomEqualizer e)
{
	pConn->Send("PSMULTEQ:" + lu(gRoomEq, e) + "\r");
}


void Command::menu(MenuControl mc)
{
	switch(mc)
	{
		case MenuControl::Up: pConn->Send("MNCUP\r"); break;
		case MenuControl::Down: pConn->Send("MNCDN\r"); break;
		case MenuControl::Left: pConn->Send("MNCLT\r"); break;
		case MenuControl::Right: pConn->Send("MNCRT\r"); break;
		case MenuControl::Enter: pConn->Send("MNENT\r"); break;
		case MenuControl::Return: pConn->Send("MNRET\r"); break;
		case MenuControl::On: pConn->Send("MNMEN ON\r"); break;
		case MenuControl::Off: pConn->Send("MNMEN OFF\r"); break;
	}
}


} // namespace Denon
