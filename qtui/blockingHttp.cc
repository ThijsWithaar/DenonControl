#include "blockingHttp.h"

#include <iostream>

#include <QHostAddress>



QHttpConnection::QHttpConnection(std::string host, int port):
	m_host(QString::fromStdString(host)),
	m_port(port),
	m_parser(Denon::Http::Method::Get)
{
}


const Denon::Http::Response& QHttpConnection::Http(const Denon::Http::Request& req)
{
	m_socket.connectToHost(m_host, m_port);
	m_socket.waitForConnected(3000);

	auto reqc = req;
	reqc.fields["HOST"] = m_host.toString().toStdString() + ":" + std::to_string(m_port);

	std::string msg = reqc;
	m_socket.write(msg.data(), msg.size());
	if(!m_socket.waitForBytesWritten())
		std::cout << "QHttpConnection: Could not write\n";

	while(m_socket.isReadable())
	{
		m_socket.waitForReadyRead();
		auto buf = m_parser.currentBuffer();
		size_t nRead = m_socket.read(buf.first, buf.second);
		if(nRead == std::numeric_limits<size_t>::max())
		{
			std::cout << "QHttpConnection: Read Error: " << m_socket.errorString().toStdString() << "\n";
			break;
		}

		if(auto pPacket = m_parser.markRead(nRead))
		{
			m_response = *pPacket;
			break;
		}
	}

	m_socket.close();
	if(m_socket.isOpen())
		m_socket.waitForDisconnected();

	return m_response;
}
