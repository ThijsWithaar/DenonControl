#pragma once

#include <Denon/network/http.h>


class DummyConnection: public Denon::Http::BlockingConnection
{
public:
	DummyConnection(std::string_view data);

	size_t read(char* dst, size_t nBuf);

	const Denon::Http::Response& Http(const Denon::Http::Request& req) override;

private:
	std::string_view m_data;
	size_t m_offset;

	// BlockingConnection interface
	Denon::Http::Parser m_parser;
	Denon::Http::Response m_response;
};
