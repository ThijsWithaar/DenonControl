#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <boost/property_tree/ptree.hpp>

#include <Denon/denon.h>
#include <Denon/network/http.h>
#include <Denon/upnpEvent.h>


/// Register to and receive uPnP events on device changes.
/// Volume changes are broadcasted.
class UpnpEvents:
		public QObject,
		public Denon::Upnp::EventHandler
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
	void wifiSsid(QString ssid);

private slots:
	void onSubsrcibeConnected();
	void onSubsrcibeDisconnected();
	void onResubsribe();

	void onCbConnection();
	void onCbRead();

private:
	void parseProperties(const boost::property_tree::ptree& pt);
	void parseEvents(const std::string& name, const boost::property_tree::ptree& pt);

	// Upnp::EventHandler
	void onDeviceName(std::string_view name) override;
	void onPower(bool on) override;
	void onVolume(Denon::Channel c, double vol) override;
	void onZoneVolume(std::string_view name, double vol) override;
	void onMute(std::string_view channel, bool muted) override;
	void wifiSsid(std::string_view ssid) override;

	QHostAddress m_deviceAddress;	// IP address of amplifier

	int m_subscribeIdx;				// Index of currently registerd event
	QTcpSocket m_subSocket;			// Subscription socket
	QTimer m_subTimer;				// Timer to re-new subscriptions

	QTcpServer m_cbSocket;			// Event callbacks, port 49200 on android app
	Denon::Http::Parser m_cbParser;	// Parser for the event callbacks
};
