#pragma once

#include <QWidget>

#include "ui_open.h"


class OpenDialog: public QDialog
{
	Q_OBJECT
public:
	OpenDialog(QWidget* parent, QString ipAddress);

	QString GetAddress();

private:
	Ui::OpenDialog ui;
};

