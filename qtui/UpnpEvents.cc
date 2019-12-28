#include "UpnpEvents.h"

#include <boost/property_tree/xml_parser.hpp>


#include <Denon/string.h>
#include <Denon/upnpEvent.h>

#include <iostream>
#include <sstream>


constexpr int listenPort = 49200;
constexpr int uPnpPort = 60006;

// From http://192.168.4.7:60006/upnp/desc/aios_device/aios_device.xml
const QString uriAct = "/ACT/event";
const QString uriAoisErr = "/upnp/event/AiosServicesDvc/ErrorHandler";
const QString uriAiosGroup = "/upnp/event/AiosServicesDvc/GroupControl";
const QString uriAiosZone = "/upnp/event/AiosServicesDvc/ZoneControl";
const QString uriDvcTrans = "/upnp/event/renderer_dvc/AVTransport";
const QString uriDvcConMan = "/upnp/event/renderer_dvc/ConnectionManager";
const QString uriDvcRC = "/upnp/event/renderer_dvc/RenderingControl";
// /upnp/event/ams_dvc/ContentDirectory
// /upnp/event/ams_dvc/ConnectionManager

const std::vector<QString> uris = {uriDvcTrans, uriDvcConMan, uriDvcRC, uriAct};


UpnpEvents::UpnpEvents()
{
	connect(&m_cbSocket, &QTcpServer::newConnection, this, &UpnpEvents::onCbConnection);
	m_cbSocket.listen(QHostAddress::Any, listenPort);

	connect(&m_subbSocket, &QTcpSocket::connected, this, &UpnpEvents::onSubsrcibeConnected);
	connect(&m_subbSocket, &QTcpSocket::disconnected, this, &UpnpEvents::onSubsrcibeDisconnected);
}


void UpnpEvents::Register(QHostAddress addr)
{
	m_deviceAddress = addr;
	registerIdx = 0;
	onSubsrcibeDisconnected();
}


void UpnpEvents::onSubsrcibeDisconnected()
{
	//std::cout << "UpnpEvents::onSubsrcibeDisconnected\n";
	if(registerIdx < uris.size())
		m_subbSocket.connectToHost(m_deviceAddress, uPnpPort);
}


void UpnpEvents::onSubsrcibeConnected()
{
	auto deviceAddr = m_subbSocket.peerAddress().toString();
	auto localAddr = m_subbSocket.localAddress().toString();

	Upnp::Subscribe sub;
	sub.host = deviceAddr.toStdString() + ":" + std::to_string(uPnpPort);
	sub.callback = "<http://" + localAddr.toStdString() + ":" + std::to_string(listenPort) + "/>";
	sub.type = "upnp:event";
	sub.timeOut = "Second-180";

	if(registerIdx < uris.size())
	{
		sub.url = uris[registerIdx++].toStdString();
		std::string msg = sub;
		//std::cout << "UpnpEvents::onSubsrcibeConnection\n" << msg << "\n";
		m_subbSocket.write(msg.data(), msg.size());
		m_subbSocket.close();
	}
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
		onCbProperty(pt);

		Upnp::Response resp(200);
		resp.fields["CONNECTION"] = "close";
		resp.fields["CONTENT-TYPE"] = "text/html";
		resp.body = "<html><body><h1>200 OK</h1></body></html>";
		std::string rs = resp;
		//std::cout << "Response\n" << rs;
		socket->write(rs.data(), rs.size());
	}
}


void UpnpEvents::onCbProperty(const boost::property_tree::ptree& pt)
{
	if(auto ps = pt.get_child_optional("e:propertyset"))
	{
		for(auto& prop: *ps)
		{
			if(auto friendlyName = prop.second.get_child_optional("LastChange.Event.FriendlyName"))
			{
				auto val = friendlyName->get<std::string>("<xmlattr>.val");
				std::cout << "Friendly Device name: " << val << "\n";
				emit deviceName(QString::fromStdString(val));
			}
			else if(auto devPower = prop.second.get_child_optional("LastChange.Event.DevicePower"))
			{
				auto val = devPower->get<std::string>("<xmlattr>.val");
				emit power(val == "ON");
			}
			else if(auto iid = prop.second.get_child_optional("LastChange.Event.InstanceID"))
			{
				for(auto& [key, val]: *iid)
				{
					if(key == "Volume")
					{
						auto channel = val.get<std::string>("<xmlattr>.channel");
						auto vval = val.get<double>("<xmlattr>.val");
						if(channel == "Master")
							emit volumeChanged(Denon::Channel::Master, vval);
						else
							zoneVolumeChanged(QString::fromStdString(channel), vval);
					}
					else if(key == "Bass")
						emit volumeChanged(Denon::Channel::Bass, val.get<double>("<xmlattr>.val"));
					else if(key == "Treble")
						emit volumeChanged(Denon::Channel::Treble, val.get<double>("<xmlattr>.val"));
					else if(key == "Subwoofer")
						emit volumeChanged(Denon::Channel::Sub, val.get<double>("<xmlattr>.val"));
				}
			}
		} // property
	} // propertyset
}
