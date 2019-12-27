#pragma once

#include <Denon/denon.h>
#include <Denon/property.h>


namespace Denon {

constexpr int TelnetPort = 23;

/// Parse string, call `response`
void ParseResponse(std::string str, Response& response);

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

	void Power(bool);
	void Mute(bool);
	void Volume(Channel c, int volume);
	void Input(Source s);
	void Surround(Surround s);
	void RoomEq(RoomEqualizer e);	// Super-seded by MultiEq() ?
	void DynamicEq(bool on);
	void DynamicVolume(DynamicVolume v);
	void CinemaEq(bool on);
	void MultiEq(RoomEqualizer e);

	void menu(MenuControl mc);
private:
	CommandConnection* pConn;
};


} // namespace Denon
