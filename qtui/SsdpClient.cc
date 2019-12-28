#include "SsdpClient.h"

#include <iostream>

#include <QUrl>

#include <Denon/ssdp.h>


const QHostAddress ssdpHost = QHostAddress("239.255.255.250");
const int ssdpPort = 1900;


SsdpClient::SsdpClient(QObject* parent)
{
	m_socket.bind(QHostAddress::AnyIPv4, ssdpPort, QUdpSocket::ReuseAddressHint);
	m_socket.joinMulticastGroup(ssdpHost);

	connect(&m_socket, &QUdpSocket::readyRead, this, &SsdpClient::onRead);
}


void SsdpClient::scan()
{
	Ssdp::Search s;
	s.searchType = "urn:schemas-denon-com:device:ACT-Denon:1";
	std::string msg = s;
	m_socket.writeDatagram(msg.data(), msg.size(), ssdpHost, ssdpPort);
}


void SsdpClient::onRead()
{
	while (m_socket.hasPendingDatagrams())
	{
		auto sz = m_socket.readDatagram(m_rxBuf.data(), m_rxBuf.size());
		std::string_view msg(m_rxBuf.data(), sz);

		auto sn = Ssdp::Parse(msg);
		if(auto pNotify = std::get_if<Ssdp::Notify>(&sn))
		{
			QUrl url(QString::fromStdString(pNotify->location));
			//std::cout << "SsdpClient::onRead Notify " << pNotify->location << "\n";
			emit deviceFound(QString::fromStdString(pNotify->nt), QHostAddress(url.host()));
		}
	}
};
