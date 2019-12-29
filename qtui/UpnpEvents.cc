#include "UpnpEvents.h"

#include <boost/property_tree/xml_parser.hpp>


#include <Denon/string.h>
#include <Denon/upnpEvent.h>

#include <iostream>
#include <sstream>


constexpr int listenPort = 49200;
constexpr int uPnpPort = 60006;
constexpr int subscriptionTimeout_sec = 180;

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

	Upnp::Subscribe sub;
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
	if(auto pPacket = m_cbParser.markRead(nRead))
	{
		//std::cout << "Decoded:\n" << urlDecode(pPacket->body) << std::endl;

		std::stringstream bodyss;
		bodyss << urlDecode(pPacket->body);
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(bodyss, pt);
		parseProperties(pt);

		Upnp::Response resp(200);
		resp.fields["CONNECTION"] = "close";
		resp.fields["CONTENT-TYPE"] = "text/html";
		resp.body = "<html><body><h1>200 OK</h1></body></html>";
		std::string rs = resp;
		//std::cout << "Response\n" << rs;
		socket->write(rs.data(), rs.size());
	}
}


void UpnpEvents::parseProperties(const boost::property_tree::ptree& pt)
{
	if(auto events = pt.get_child_optional("e:propertyset.e:property.LastChange.Event"))
	{
		for(auto& event: *events)
		{
			parseEvents(event.first, event.second);
		}
	}
}


void UpnpEvents::parseEvents(const std::string& name, const boost::property_tree::ptree& pt)
{
	if(name == "FriendlyName")
	{
		auto val = pt.get<std::string>("<xmlattr>.val");
		std::cout << "Friendly Device name: " << val << "\n";
		emit deviceName(QString::fromStdString(val));
	}
	else if(name == "DevicePower")
	{
		auto val = pt.get<std::string>("<xmlattr>.val");
		emit power(val == "ON");
	}
	else if(name == "InstanceID")
	{
		for(auto& [key, val]: pt)
		{
			if(key == "Volume")
			{
				auto channel = val.get<std::string>("<xmlattr>.channel");
				auto vval = val.get<double>("<xmlattr>.val");
				if(channel == "Master")
					emit volumeChanged(Denon::Channel::Master, vval);
				else
					emit zoneVolumeChanged(QString::fromStdString(channel), vval);
			}
			else if(key == "Bass")
				emit volumeChanged(Denon::Channel::Bass, val.get<double>("<xmlattr>.val"));
			else if(key == "Treble")
				emit volumeChanged(Denon::Channel::Treble, val.get<double>("<xmlattr>.val"));
			else if(key == "Subwoofer")
				emit volumeChanged(Denon::Channel::Sub, val.get<double>("<xmlattr>.val"));
			else if(key == "Mute")
			{
				auto channel = val.get<std::string>("<xmlattr>.channel");
				auto mval = val.get<int>("<xmlattr>.val");
				emit mute(QString::fromStdString(channel), mval != 0);
			}
		}
	}
	else if(name == "WifiApSsid")
	{
		auto mval = pt.get<std::string>("<xmlattr>.val");
		emit wifiSsid(QString::fromStdString(mval));
	}
	else if(name == "SurroundSpeakerConfig")
	{
		/*auto cfg = pt.get<std::string>("<xmlattr>.val");
		std::stringstream ss;
		ss << urlDecode(cfg);
		std::cout << "* SurroundSpeakerConfig *\n" << ss.str() << "\n";*/
	}
}
