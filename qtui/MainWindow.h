#pragma once

#include <QMainWindow>
#include <QTcpSocket>
#include <QSystemTrayIcon>
#include <QSettings>

#include <Denon/denon.h>
#include <Denon/serial.h>

#include "ui_main.h"


class Settings: public QSettings
{
public:
	Settings(QObject *parent);
	~Settings();

	void SetIpAddress(QString);
	QString GetIpAddress();
};


class MainWindow :
	public QMainWindow,
	public Denon::CommandConnection,
	public Denon::Response
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent);

private slots:
	// UI controls
	void onOpenDevice();
	void onMute(bool);
	void onVolume(int);
	void onSource(int);
	void onSurround(int);
	void onRoomEq(int);

	// Socket response
	void onResponse();

	// Generic UI
	void onShowHide(QSystemTrayIcon::ActivationReason reason);
	void changeEvent(QEvent* e) override;
	void closeEvent(QCloseEvent *event) override;

	// Telnet connection
	void ConnectTelnet();
	void RequestTelnetStatus();

private:
	// Denon::CommandConnection
	void Send(const std::string&) override;

	// Denon::Response
	void OnPower(bool) override;
	void OnMute(bool) override;
	void OnVolume(Denon::Channel c, int v) override;
	void OnInput(Denon::Source s) override;
	void OnSurround(Denon::Surround s) override;
	//void OnRoomEq(Denon::RoomEqualizer e) override;
	void OnSampleRate(int hz) override;
	void OnBluetooth(bool on) override;
	void OnDynamicEq(bool on) override;
	void OnDynamicVolume(Denon::DynamicVolume v) override;
	void OnCinemaEq(bool on) override;
	void OnMultiEq(Denon::RoomEqualizer e) override;
	void OnSpeaker(Denon::Speaker speaker, Denon::SpeakerType type) override;

private:
	void SetupTrayIcon();

	Ui::MainWindow ui;
	QSystemTrayIcon* trayIcon;
	Settings settings;
	QTcpSocket socket;
	Denon::Command command;
	QRect lastGeometry;
};
