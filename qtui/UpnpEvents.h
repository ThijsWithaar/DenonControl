#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <boost/property_tree/ptree.hpp>

#include <Denon/denon.h>
#include <Denon/upnpEvent.h>

/// Register to and receive uPnP events on device changes.
/// Volume changes are broadcasted.
class UpnpEvents: public QObject
{
	Q_OBJECT
public:
	UpnpEvents(QObject* parent=nullptr);

	/// Register to a hardcoded set of events.
	/// TO-DO: get them from the SSDP url
	void Register(QHostAddress);

signals:
	void power(bool on);
	void deviceName(QString name);
	void volumeChanged(Denon::Channel, double volume);
	void zoneVolumeChanged(QString zone, double volume);
	void mute(QString channel, bool muted);

private slots:
	void onSubsrcibeConnected();
	void onSubsrcibeDisconnected();
	void onResubsribe();

	void onCbConnection();
	void onCbRead();
	void onCbProperty(const boost::property_tree::ptree& pt);

private:
	QHostAddress m_deviceAddress;	// IP address of amplifier

	int m_subscribeIdx;				// Index of currently registerd event
	QTcpSocket m_subSocket;			// Subscription socket
	QTimer m_subTimer;				// Timer to re-new subscriptions

	QTcpServer m_cbSocket;			// Event callbacks, port 49200 on android app
	Upnp::HttpParser m_cbParser;	// Parser for the event callbacks
};
