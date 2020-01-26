#include "MainWindow.h"
#include "OpenDialog.h"

#include <iostream>


#include <QSettings>


// Break the UI<->Denon update loop
class SignalBlocker
{
public:
	SignalBlocker(QObject* obj):
		m_obj(obj)
	{
		m_obj->blockSignals(true);
	}

	~SignalBlocker()
	{
		m_obj->blockSignals(false);
	}

private:
	QObject* m_obj;
};


//-- Settings --


Settings::Settings(QObject* parent):
	QSettings(parent)
	//QSettings("denonControl", "denon", parent)
{
	//QSettings settings;
	//ipAddress = settings.value("ipAddress", "192.168.4.7").toString();
}


Settings::~Settings()
{
	//QSettings settings;
	//settings.setValue("ipAddress", ipAddress);
	std::cout << "Saving Settings: " << fileName().toStdString() << std::endl;
}


void Settings::SetIpAddress(QString ipAddress)
{
	//this->ipAddress = ipAddress;
	setValue("ipAddress", ipAddress);
}


QString Settings::GetIpAddress()
{
	//return ipAddress;
	return value("ipAddress", "192.168.4.7").toString();
}


bool operator<(const QHostAddress& a, const QHostAddress& b)
{
	return a.toIPv4Address() < b.toIPv4Address();
}


//-- MainWindow --


MainWindow::MainWindow(QWidget* parent):
	command(this),
	settings(this)
{
	ui.setupUi(this);

	connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onOpenDevice);
	connect(ui.actionQuit, &QAction::triggered, QApplication::instance(), &QApplication::quit);

	SetupCommandConnections();
	SetupTelnet();
	SetupHttp();
	SetupTrayIcon();

	// UI state
	restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}


void MainWindow::SetupCommandConnections()
{
	connect(ui.power, &QCheckBox::clicked, [&](bool b){
		command.Power(b);
	});
	connect(ui.mute, &QCheckBox::clicked, [&](bool m){
		command.Mute(m);
	});
	connect(ui.sVolume, &QSlider::valueChanged, [&](int v){
		command.Volume(Denon::Channel::Master, v);
	});
	connect(ui.cSource, qOverload<int>(&QComboBox::currentIndexChanged), [&](int s){
		command.Input(static_cast<Denon::Source>(s));
	});
	connect(ui.cSurround, qOverload<int>(&QComboBox::currentIndexChanged), [&](int s){
		command.Surround(static_cast<Denon::Surround>(s));
	});
	connect(ui.cDynEq, &QCheckBox::clicked, [&](bool b){
		command.DynamicEq(b);
	});
	connect(ui.cDynVol, qOverload<int>(&QComboBox::currentIndexChanged), [&](int e){
		command.DynamicVolume(static_cast<Denon::DynamicVolume>(e));
	});
	connect(ui.cCinemaEq, &QCheckBox::clicked, [&](bool b){
		command.CinemaEq(b);
	});
	connect(ui.cMultiEq, qOverload<int>(&QComboBox::currentIndexChanged), [&](int e){
		command.MultiEq(static_cast<Denon::RoomEqualizer>(e));
	});
	connect(ui.cEco, qOverload<int>(&QComboBox::currentIndexChanged), [&](int e){
		command.EcoMode(static_cast<Denon::EcoMode>(e));
	});
	connect(ui.cSoundMode, qOverload<int>(&QComboBox::currentIndexChanged), [&](int e){
		command.SoundMode(static_cast<Denon::SoundMode>(e));
	});
}


void MainWindow::SetupTelnet()
{
	// Telnet connection
	connect(&m_telnet, &QTcpSocket::readyRead, this, &MainWindow::onResponse);
	connect(&m_telnet, &QTcpSocket::disconnected, this, &MainWindow::ConnectTelnet);
	connect(&m_telnet, &QTcpSocket::connected, this, &MainWindow::RequestTelnetStatus);
	ConnectTelnet();
}


void MainWindow::SetupHttp()
{
	// Device scan on network
	connect(&m_ssdp, &SsdpClient::deviceFound, [&](QString type, QHostAddress addr)
	{
		if(m_devices.count(addr) == 0)
			std::cout << "Found SSDP device: " << type.toStdString() << " at " << addr.toString().toStdString() << "\n";
		m_devices.insert(addr);
	});
	m_ssdp.scan();

	// HTTP connection
	m_upnp.Register(QHostAddress(settings.GetIpAddress()));
	connect(&m_upnp, &UpnpEvents::zoneVolumeChanged, [&](QString zone, double val)
	{
		std::cout << "Upnp zone vol changed : " << zone.toStdString() << ", " << val << "\n";
	});
	connect(&m_upnp, &UpnpEvents::volumeChanged, [&](Denon::Channel c, double val)
	{
		std::cout << "Upnp vol changed : " << (int)c << ", " << val << "\n";
	});
	connect(&m_upnp, &UpnpEvents::power, [&](bool p)
	{
		SignalBlocker block(ui.power);
		ui.power->setCheckState(p ? Qt::Checked : Qt::Unchecked);
	});
	connect(&m_upnp, &UpnpEvents::mute, [&](QString zone, bool m)
	{
		std::cout << "Upnp mute: " << zone.toStdString() << " : " << (int)m << "\n";
	});
}


void MainWindow::onOpenDevice()
{
	m_ssdp.scan();

	std::cout << "MainWindow::onOpenDevice: " << m_telnet.state() << std::endl;
	auto open = new OpenDialog(this, settings.GetIpAddress());

	//open->SetKnownDevices(m_devices);
	//connect(&m_ssdp, &SsdpClient::deviceFound, open, &OpenDialog::onDeviceFound);

	if(open->exec() == QDialog::Accepted)
	{
		settings.SetIpAddress(open->GetAddress());
		bool needsConnect = m_telnet.state() != QTcpSocket::SocketState::ConnectedState;
		m_telnet.close();
		if(needsConnect)
			ConnectTelnet();
	}

	// disconnect(deviceFound) ?
}


void MainWindow::ConnectTelnet()
{
	auto deviceAddr = settings.GetIpAddress();
	std::cout << "MainWindow::ConnectTelnet : " << deviceAddr.toStdString() << "\n";
	m_telnet.connectToHost(deviceAddr, Denon::TelnetPort);
}


void MainWindow::RequestTelnetStatus()
{
	m_telnet.setTextModeEnabled(true);
	command.RequestStatus();
}


void MainWindow::SetupTrayIcon()
{
	trayIcon = new QSystemTrayIcon(QIcon(":/denon.png"), this);

	connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onShowHide);

	QAction *quit_action = new QAction( "Quit", trayIcon );
	quit_action->setIcon(QIcon::fromTheme("application-exit"));
	connect( quit_action, &QAction::triggered, this, &QApplication::quit);

#if 0
	QAction *hide_action = new QAction( "Show/Hide", trayIcon );
	hide_action->setIcon(QIcon::fromTheme("view-restore"));
	connect( hide_action, &QAction::triggered, [this]()
	{	this->onShowHide(QSystemTrayIcon::ActivationReason::Trigger); } );
#endif

	QAction *muteAction = new QAction( "Mute", trayIcon );
	muteAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
	connect( muteAction, &QAction::triggered, [this, muteAction]()
	{
		bool doMute = ui.mute->checkState() != Qt::Checked;
		this->OnMute(doMute);
		command.Mute(doMute);
		muteAction->setChecked(doMute);
	});

	QMenu *tray_icon_menu = new QMenu;
	tray_icon_menu->addAction( muteAction );
	//tray_icon_menu->addAction( hide_action );
	tray_icon_menu->addAction( quit_action );

	trayIcon->setContextMenu( tray_icon_menu );
	trayIcon->show();
}


void MainWindow::onShowHide(QSystemTrayIcon::ActivationReason reason)
{
	//std::cout << "onShowHide : visible " << isVisible() << ", last.left " << lastGeometry.left() << std::endl;
	if( isVisible() )
	{
		if(!isMinimized())
		{
			lastGeometry = geometry();
			//std::cout << "lastGeometry : " << lastGeometry.left() << "\n";
		}
		hide();
	}
	else
	{
		show();
		//std::cout << "onShowHide() setGeometry : " << lastGeometry.left() << "\n";
		setGeometry(lastGeometry);
		raise();
		setFocus();
	}
}


void MainWindow::changeEvent(QEvent* e)
{
	if(!isMinimized() && isVisible())
	{
		//lastGeometry = geometry();
		//std::cout << "changeEvent.lastLeft : " << lastGeometry.left() << "\n";
	}
	//std::cout << "changeEvent: left : " << geometry().left() << ", visible " << isVisible()<<
	//	", minimized " << isMinimized() <<  ", eventType " << e->type() << "\n";

	switch (e->type())
	{
		case QEvent::Hide:
		case QEvent::Close:
			//std::cout << "changeEvent close: lastLeft : " << lastGeometry.left() << std::endl;
			//lastGeometry = geometry();
			settings.setValue("windowGeometry", saveGeometry());
			settings.setValue("windowState", saveState());
			break;
		case QEvent::WindowStateChange:
			//std::cout << "changeEvent : minimized " << isMinimized() << std::endl;
			if(isMinimized())
			{
				lastGeometry = geometry();
				e->accept();
				hide();
			}
			else
			{
				setGeometry(lastGeometry);
			}
			break;
		default:
			break;
	}

	//e->ignore();
	//QMainWindow::changeEvent(e);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
	std::cout << "MainWindow::closeEvent" << std::endl;
	settings.setValue("windowGeometry", saveGeometry());
	settings.setValue("windowState", saveState());
	QMainWindow::closeEvent(event);
}


//-- UI control --


void MainWindow::onResponse()
{
	auto nRead = m_telnet.readLine(m_telnetRxBuf.data(), m_telnetRxBuf.size());
	std::string_view msg(m_telnetRxBuf.data(), nRead);
	{
		std::string s{msg};
		std::replace( s.begin(), s.end(), '\r', ',');
		s.erase(s.find_last_not_of(',') + 1);
		std::cout << "Denon: " << s << "\n";
	}
	statusBar()->showMessage(QString::fromLocal8Bit(msg.data(), (int)msg.size()));

	try
	{
		Denon::ParseResponse(msg, *this);
	}
	catch(std::exception& e)
	{
		std::throw_with_nested(std::runtime_error("Serial Parsing of:" + std::string(msg) + "\n"));
	}
}


//-- Denon control --


void MainWindow::Send(const std::string& cmd)
{
	{
		std::string s{cmd};
		std::replace( s.begin(), s.end(), '\r', ',');
		s.erase(s.find_last_not_of(',') + 1);
		std::cout << "Ui: " << s << "\n";
	}
	if(m_telnet.isWritable())
		m_telnet.write(cmd.data(), cmd.size());
}


void MainWindow::OnPower(bool p)
{
	SignalBlocker block(ui.power);
	ui.power->setCheckState(p ? Qt::Checked : Qt::Unchecked);
}


void MainWindow::OnMute(bool muted)
{
	SignalBlocker block(ui.mute);
	ui.mute->setCheckState(muted ? Qt::Checked : Qt::Unchecked);
}


void MainWindow::OnVolume(Denon::Channel c, int v)
{
	if(c != Denon::Channel::Master)
	{
		//std::cout << "OnVolume " + std::to_string(v) << "\n";
		return;
	}
	SignalBlocker block(ui.sVolume);
	ui.sVolume->setValue(v);
}


void MainWindow::OnInput(Denon::Source s)
{
	SignalBlocker block(ui.cSource);
	//std::cout << "MainWindow::OnInput " << (int)s << "\n";
	ui.cSource->setCurrentIndex((int)s);
}


/*void MainWindow::OnRoomEq(Denon::RoomEqualizer e)
{
	SignalBlocker block(ui.cRoomEq);
	ui.cRoomEq->setCurrentIndex((int)e);
}*/


void MainWindow::OnSurround(Denon::Surround s)
{
	SignalBlocker block(ui.cSurround);
	//std::cout << "MainWindow::OnSurround " << (int)s << "\n";
	ui.cSurround->setCurrentIndex((int)s);
}


void MainWindow::OnSampleRate(int hz)
{
	std::cout << "MainWindow::OnSampleRate " << (int)hz << "\n";
}


void MainWindow::OnBluetooth(bool on)
{
	std::cout << "MainWindow::OnBluetooth " << (int)on << "\n";
}


void MainWindow::OnDynamicEq(bool v)
{
	SignalBlocker block(ui.cDynEq);
	ui.cDynEq->setCheckState(v ? Qt::Checked : Qt::Unchecked);
}


void MainWindow::OnDynamicVolume(Denon::DynamicVolume v)
{
	SignalBlocker block(ui.cDynVol);
	ui.cDynVol->setCurrentIndex((int)v);
}


void MainWindow::OnCinemaEq(bool v)
{
	SignalBlocker block(ui.cCinemaEq);
	ui.cCinemaEq->setCheckState(v ? Qt::Checked : Qt::Unchecked);
}


void MainWindow::OnMultiEq(Denon::RoomEqualizer e)
{
	SignalBlocker block(ui.cMultiEq);
	ui.cMultiEq->setCurrentIndex((int)e);
}


void MainWindow::OnSpeaker(Denon::Speaker speaker, Denon::SpeakerType type)
{
}


void MainWindow::OnEco(Denon::EcoMode mode)
{
	SignalBlocker block(ui.cEco);
	ui.cEco->setCurrentIndex((int)mode);
}


void MainWindow::OnSoundMode(Denon::SoundMode mode)
{
	ui.cDynEq->setEnabled(mode != Denon::SoundMode::Pure);
	ui.cDynVol->setEnabled(mode != Denon::SoundMode::Pure);
	ui.cMultiEq->setEnabled(mode != Denon::SoundMode::Pure);

	SignalBlocker block(ui.cSoundMode);
	ui.cSoundMode->setCurrentIndex((int)mode);
}
