#pragma once
/*
Wrapper for the Denon protocol

https://usa.denon.com/us/product/hometheater/receivers/avr3808ci?docname=AVR-3808CISerialProtocol_Ver520a.pdf
*/


namespace Denon {


enum class Channel
{
	Master, FrontLeft, Center, Balance, Sub,
	LFE, Bass, Treble
};

enum class Source
{
	TUNER, PHONO, CD, DVD, NET_USB, MediaPlayer
	// HDP, TV_CBL, SAT, VCR, V_AUX, , XM, IPOD
};

enum class Surround
{
	Direct, PureDirect, Stereo, Standard,
	DolbyDigital, DolbyDigitalNeuralX, DtsSurround,
	Neural, WideScreen, _7ChStereo,
	SuperStadium, RockArena, JazzClub, Classic, MonoMovie, Matrix, VideoGame
};

enum class RoomEqualizer
{
	Audyssey, Bypass, Flat, Manual, Off
};

enum class DynamicVolume
{
	Off, Light, Medium, Heavy
};

enum class Speaker
{
	Front, Center, Sua, Sbk, Frh, Tfr, Tpm, Frd, Sud, SubWoofer
};

enum class SpeakerType
{
	None, TwoSpeaker, Small, Large
};

enum class EcoMode
{
	Off, On, Auto
};

enum class SoundMode
{
	Pure, Music, Movie, Game
};

enum class DrcMode
{
	Auto, Low, Mid, Hi, Off
};

/// Interface for responses
class Response
{
public:
	virtual void OnPower(bool on) = 0;
	virtual void OnMute(bool muted) = 0;
	virtual void OnVolume(Channel c, int v) = 0;
	virtual void OnInput(Source s) = 0;
	virtual void OnSurround(Surround s) = 0;
	//virtual void OnRoomEq(RoomEqualizer e) = 0;
	virtual void OnSampleRate(int hz) = 0;
	virtual void OnBluetooth(bool on) = 0;
	virtual void OnDynamicEq(bool on) = 0;
	virtual void OnDynamicVolume(DynamicVolume v) = 0;
	virtual void OnDynamicRangeControl(DrcMode v) = 0;
	virtual void OnCinemaEq(bool on) = 0;
	virtual void OnMultiEq(Denon::RoomEqualizer e) = 0;
	virtual void OnSpeaker(Denon::Speaker speaker, Denon::SpeakerType type) = 0;
	virtual void OnEco(EcoMode mode) = 0;
	virtual void OnSoundMode(SoundMode mode) = 0;
};

} // namespace Denon
