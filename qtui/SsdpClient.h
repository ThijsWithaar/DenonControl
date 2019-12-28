#pragma once

#include <array>

#include <QUdpSocket>


/// Client to search for Denon devices on the network, using UPnP (SSDP protocol)
class SsdpClient: public QObject
{
	Q_OBJECT
public:
	SsdpClient(QObject* parent = nullptr);

public slots:
	void scan();		///> Scan the network for denon devices

signals:
	void deviceFound(QString, QHostAddress);

private:
	void onRead();

	QUdpSocket m_socket;
	std::array<char, 1<<10> m_rxBuf;
};

