#pragma once

#include <QTcpServer>
#include <QTcpSocket>

#include <boost/property_tree/ptree.hpp>

#include <Denon/denon.h>
#include <Denon/upnpEvent.h>

/*
	Volume info comes from:
	SUBSCRIBE /upnp/event/renderer_dvc/RenderingControl HTTP/1.1\r\n
*/

/// Register to and receive uPnP events on device changes.
/// Volume changes are broadcasted.
class UpnpEvents: public QObject
{
	Q_OBJECT
public:
	UpnpEvents();

	/// Register to a hardcoded set of events.
	/// TO-DO: get them from the SSDP url
	void Register(QHostAddress);

signals:
	void power(bool);
	void deviceName(QString);
	void volumeChanged(Denon::Channel, double);
	void zoneVolumeChanged(QString, double);

private:
	void onSubsrcibeConnected();
	void onSubsrcibeDisconnected();

	void onCbConnection();
	void onCbRead();
	void onCbProperty(const boost::property_tree::ptree& pt);

	int registerIdx;				// Index of currently registerd event
	QHostAddress m_deviceAddress;	// IP address of amplifier
	QTcpSocket m_subbSocket;		// Subscription socket

	QTcpServer m_cbSocket;			// Event callbacks, port 49200 on android app
	Upnp::HttpParser m_cbParser;	// Parser for the event callbacks
};
