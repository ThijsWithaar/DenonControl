#pragma once

#include <QTcpSocket>
#include <QHostAddress>

#include <Denon/http.h>


class QHttpConnection: public Denon::Http::BlockingConnection
{
public:
	QHttpConnection(std::string host, int port);

	const Denon::Http::Response& Http(const Denon::Http::Request& req) override;

private:
	QHostAddress m_host;
	int m_port;
	QTcpSocket m_socket;
	Denon::Http::Parser m_parser;
	Denon::Http::Response m_response;
};
