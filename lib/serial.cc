#include <Denon/serial.h>
#include <Denon/string.h>

#include <iostream>
#include <functional>
#include <map>
#include <ranges>
#include <sstream>
#include <string_view>


namespace Denon {



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
	{"DOLBY AUDIO-DD+ +NEURAL:X", Surround::DolbyDigitalNeuralX},
	{"M CH IN+DSUR", Surround::DtsSurround},
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


const std::map<std::string, EcoMode> gEcoMode = {
	{"OFF", EcoMode::Off},
	{"ON", EcoMode::On},
	{"AUTO", EcoMode::Auto},
};


const std::map<std::string, SoundMode> gSoundMode = {
	{"PUR", SoundMode::Pure},
	{"MUS", SoundMode::Music},
	{"MOV", SoundMode::Movie},
	{"GAM", SoundMode::Game},
};


const std::map<std::string, DrcMode> gDynamicRangeControl = {
	{"AUTO", DrcMode::Auto},
	{"LOW", DrcMode::Low},
	{"MID", DrcMode::Mid},
	{"HI", DrcMode::Hi},
	{"OFF", DrcMode::Off},
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


void ParseResponse(std::string_view lines, Response& response)
{
	if(lines.size() < 3)
		return;

	std::string param;

	const std::map<std::string, std::function<void(void)>> handlers = {
		{"BT"  , [&]()
			{
				if(param.substr(2) == "TX")
					response.OnBluetooth(param == "TX ON");
			}},
		{"ECO"  , [&](){
			auto emode = param.substr(1);
			if(gEcoMode.count(emode) == 0)
				throw std::runtime_error("Eco mode not found: " + emode);
			response.OnEco(gEcoMode.at(emode));
		}},
		{"PW"   , [&](){ response.OnPower(param == "ON"); }},		// See also 'ZM'
		{"MU"   , [&](){ response.OnMute(param == "ON"); }},
		{"MV", [&]()
			{
				if(!startswith(param, "MAX"))
				{
					int vol = std::stoi(param);
					if(vol > 100)
						vol /= 10;		// Denon seems to have implicit comma a start (0..1) values
					response.OnVolume(Channel::Master, vol);
				}
			}},
		{"MS", [&]()
			{
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
				if(param == "INFAISFSV")
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
						response.OnSpeaker(gSpeaker.at(spkKey), gSpeakerType.at(spkVal));
				}
				else if(startswith(param, "SMG"))
				{
					auto val = param.substr(4);
					response.OnSoundMode(gSoundMode.at(val));
				}
			}},
		{"PS", [&]()
			{
				if(startswith(param, "CINEMA EQ."))
				{
					response.OnCinemaEq(param == "CINEMA EQ.ON");
				}
				else if(startswith(param, "DRC"))
				{
					auto val = param.substr(4);
					if(gDynamicRangeControl.count(val))
						response.OnDynamicRangeControl(gDynamicRangeControl.at(val));
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
					std::string_view eq = param.substr(8);
					std::cout << "ROOM EQ : '" << eq << "'\n";
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
				else if(startswith(param, "MODE"))
				{
					auto val = param.substr(std::string_view("MODE").size() + 1);
					std::cout << "MODE : '" << val << "'\n";
				}
				else if(startswith(param, "MULTEQ:"))
				{
					auto val = param.substr(7);
					if(gRoomEq.count(val))
						response.OnMultiEq(gRoomEq.at(val));
				}
				else if(startswith(param, "TONE"))
				{
					auto val = param.substr(std::string_view("TONE").size() + 1);
				}
			}},
		{"ZM", [&](){ response.OnPower(param == "ON"); }},
	};

	std::string cmd;
	for(auto line: lines | std::views::split('\r'))
	{
		for (auto cmd_r : line | std::views::split(','))
		{
			if(cmd_r.size() < 3) continue;
			cmd.assign(std::begin(cmd_r), std::end(cmd_r)); // cmd_r is contiguous, but the view-types don't know that.
			param = cmd.substr(2);
			for(auto& h: handlers)
			{
				if(startswith(cmd, h.first))
				{
					h.second();
					break;
				}
			}
		}
	}
}


//-- Command --


Command::Command(CommandConnection* conn):
	pConn(conn)
{
}


void Command::RequestStatus()
{
	pConn->Send(
		"PW?\r"
		"MU?\r"
		"MV?\r"
		"SI?\r"
		"MS?\r"
		"PSDYNEQ ?\r"
		"PSDYNVOL ?\r"
		"PSCINEMA EQ. ?\r"
		"PSMODE: ?\r"
		"PSMULTEQ: ?\r"
		"ECO?\r"
		"SSSMG ?\r"
		"ZM?\r"
	);
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


void Command::DynamicRangeControl(DrcMode v)
{
	pConn->Send("PSDRC " + lu(gDynamicRangeControl, v) + "\r");
}


void Command::CinemaEq(bool v)
{
	pConn->Send(v ? "PSCINEMA EQ.ON\r" : "PSCINEMA EQ.OFF\r");
}


void Command::MultiEq(RoomEqualizer e)
{
	pConn->Send("PSMULTEQ:" + lu(gRoomEq, e) + "\r");
}


void Command::EcoMode(Denon::EcoMode e)
{
	pConn->Send("ECO" + lu(gEcoMode, e) + "\r");
}


void Command::SoundMode(Denon::SoundMode m)
{
	// This doesn't seem to do it, neither with ':' or without ' '
	/*pConn->Send("SSSMG" + lu(gSoundMode, m) + "\r");
	pConn->Send("SSSMG:" + lu(gSoundMode, m) + "\r");
	pConn->Send("SSSMG " + lu(gSoundMode, m) + "\r");
	pConn->Send("SSSMG: " + lu(gSoundMode, m) + "\r");
	*/
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
