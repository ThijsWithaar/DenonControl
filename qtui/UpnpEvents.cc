#include "UpnpEvents.h"

#include <boost/property_tree/xml_parser.hpp>


#include <Denon/http.h>
#include <Denon/string.h>
#include <Denon/upnpEvent.h>

#include <iostream>
#include <sstream>


constexpr int listenPort = 49200;
constexpr int uPnpPort = 60006;
constexpr int subscriptionTimeout_sec = 5*60;

// From http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
const QString uriAct = "/ACT/event";
const QString uriAoisErr = "/upnp/event/AiosServicesDvc/ErrorHandler";
const QString uriAiosGroup = "/upnp/event/AiosServicesDvc/GroupControl";
const QString uriAiosZone = "/upnp/event/AiosServicesDvc/ZoneControl";
const QString uriDvcTrans = "/upnp/event/renderer_dvc/AVTransport";
const QString uriDvcConMan = "/upnp/event/renderer_dvc/ConnectionManager";
const QString uriDvcRC = "/upnp/event/renderer_dvc/RenderingControl";	// This is where volume changes come from
// /upnp/event/ams_dvc/ContentDirectory
// /upnp/event/ams_dvc/ConnectionManager

const std::vector<QString> uris = {
	uriDvcTrans, uriDvcConMan, uriDvcRC,
	uriAct,
	uriAoisErr, uriAiosGroup, uriAiosZone
};


UpnpEvents::UpnpEvents(QObject* parent):
	QObject(parent),
	m_subTimer(this)
{
	connect(&m_cbSocket, &QTcpServer::newConnection, this, &UpnpEvents::onCbConnection);
	m_cbSocket.listen(QHostAddress::Any, listenPort);

	connect(&m_subSocket, &QTcpSocket::connected, this, &UpnpEvents::onSubsrcibeConnected);
	connect(&m_subSocket, &QTcpSocket::disconnected, this, &UpnpEvents::onSubsrcibeDisconnected);

	connect(&m_subTimer, &QTimer::timeout, this, &UpnpEvents::onResubsribe);
	m_subTimer.start(subscriptionTimeout_sec * 1000);
}


void UpnpEvents::Register(QHostAddress addr)
{
	m_deviceAddress = addr;
	onResubsribe();
}


void UpnpEvents::onResubsribe()
{
	if(m_deviceAddress.isNull())
		return;
	std::cout << "** UpnpEvents::onResubsribe\n";
	m_subscribeIdx = 0;
	onSubsrcibeDisconnected();
}


void UpnpEvents::onSubsrcibeDisconnected()
{
	if(m_subscribeIdx < uris.size())
		m_subSocket.connectToHost(m_deviceAddress, uPnpPort);
}


void UpnpEvents::onSubsrcibeConnected()
{
	if(m_subscribeIdx >= uris.size())
		return;

	auto deviceAddr = m_subSocket.peerAddress().toString();
	auto localAddr = m_subSocket.localAddress().toString();

	Denon::Upnp::Subscribe sub;
	sub.host = deviceAddr.toStdString() + ":" + std::to_string(uPnpPort);
	sub.callback = "<http://" + localAddr.toStdString() + ":" + std::to_string(listenPort) + "/>";
	sub.type = "upnp:event";
	sub.timeOut = "Second-" + std::to_string(subscriptionTimeout_sec);

	sub.url = uris[m_subscribeIdx++].toStdString();
	std::string msg = sub;
	m_subSocket.write(msg.data(), msg.size());
	m_subSocket.close();
}


void UpnpEvents::onCbConnection()
{
	while (m_cbSocket.hasPendingConnections())
	{
		QTcpSocket* pSocket = m_cbSocket.nextPendingConnection();
		//std::cout << "UpnpEvents: new connection from port " << std::to_string(pSocket->peerPort()) << "\n";
		connect(pSocket, &QTcpSocket::readyRead, this, &UpnpEvents::onCbRead);
		connect(pSocket, &QTcpSocket::disconnected, pSocket, &QTcpSocket::deleteLater);
	}
}


void UpnpEvents::onCbRead()
{
	QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

	auto dst = m_cbParser.currentBuffer();
	auto nRead = socket->read(dst.first, dst.second);
	while(auto pPacket = m_cbParser.markRead(nRead))
	{
		nRead = 0;
		Denon::Upnp::EventParser parser;
		parser(pPacket->body, *this);

		Denon::Http::BlockingConnection::Response resp;
		resp.status = 200;
		resp.fields["CONNECTION"] = "close";
		resp.fields["CONTENT-TYPE"] = "text/html";
		resp.body = "<html><body><h1>200 OK</h1></body></html>";
		std::string rs = resp;
		//std::cout << "Response\n" << rs;
		socket->write(rs.data(), rs.size());
	}
}


void UpnpEvents::onDeviceName(std::string_view name)
{
	emit deviceName(QString::fromStdString(std::string(name)));
}


void UpnpEvents::onPower(bool on)
{
	emit power(on);
}


void UpnpEvents::onVolume(Denon::Channel c, double vol)
{
	emit volumeChanged(c, vol);
}


void UpnpEvents::onZoneVolume(std::string_view name, double vol)
{
	emit zoneVolumeChanged(QString::fromStdString(std::string(name)), vol);
}


void UpnpEvents::onMute(std::string_view channel, bool muted)
{
	emit mute(QString::fromStdString(std::string(channel)), muted);
}


void UpnpEvents::wifiSsid(std::string_view ssid)
{
	emit wifiSsid(QString::fromStdString(std::string(ssid)));
}
