#include "OpenDialog.h"


OpenDialog::OpenDialog(QWidget* parent, QString ipAddress):
	QDialog(parent)
{
	ui.setupUi(this);
	ui.tIpAddress->setText(ipAddress);
}


QString OpenDialog::GetAddress()
{
	return ui.tIpAddress->toPlainText();
}
