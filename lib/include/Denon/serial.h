#pragma once

#include <string>
#include <string_view>

#include <Denon/denon.h>


namespace Denon {

constexpr int TelnetPort = 23;

/// Parse string, call `response`
void ParseResponse(std::string_view line, Response& response);

/// Send single commands:
class CommandConnection
{
public:
	virtual void Send(const std::string&) = 0;
};

/// Asynchronous commands
class Command
{
public:
	enum class MenuControl {
		Up, Down, Left, Right, Enter, Return, On, Off
	};

	Command(CommandConnection* conn);

	void RequestStatus();
	void Power(bool on);
	void Mute(bool muted);
	void Volume(Channel c, int volume);
	void Input(Source s);
	void Surround(Surround s);
	void RoomEq(RoomEqualizer e);	// Superseded by MultiEq() ?
	void DynamicEq(bool on);
	void DynamicVolume(DynamicVolume v);
	void CinemaEq(bool on);
	void MultiEq(RoomEqualizer e);
	void EcoMode(EcoMode e);
	void SoundMode(Denon::SoundMode m);

	void menu(MenuControl mc);
private:
	CommandConnection* pConn;
};


} // namespace Denon
