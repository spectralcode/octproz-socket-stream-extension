/**
**  This file is part of SocketStreamExtension for OCTproZ.
**  Copyright (C) 2020 Miroslav Zabic
**
**  SocketStreamExtension is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program. If not, see http://www.gnu.org/licenses/.
**
****
** Author:	Miroslav Zabic
** Contact:	zabic
**			at
**			iqo.uni-hannover.de
****
**/

#include "socketstreamextensionform.h"
#include "ui_socketstreamextensionform.h"

SocketStreamExtensionForm::SocketStreamExtensionForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SocketStreamExtensionForm)
{
	qRegisterMetaType<SocketStreamExtensionParameters >("SocketStreamExtensionParameters");
	this->ui->setupUi(this);

	//init combo box
	ui->comboBox_mode->addItem("TCP/IP", QVariant::fromValue(this->toInt(CommunicationMode::TCPIP)));
	#ifdef Q_OS_WIN
	ui->comboBox_mode->addItem("IPC - Named Pipes", QVariant::fromValue(this->toInt(CommunicationMode::IPC)));
	#else
	ui->comboBox_mode->addItem("IPC - Unix Domain Sockets", QVariant::fromValue(this->toInt(CommunicationMode::IPC)));
	#endif
	connect(ui->comboBox_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SocketStreamExtensionForm::updateGuiAccordingConnectionMode);
	this->updateGuiAccordingConnectionMode();

	//init oter gui elements
	this->initValidators();
	this->findGuiElements();
	this->connectGuiElementsToUpdateParams();
	this->enableButtonsForBroadcastingEnabledState(false);

	connect(this->ui->pushButton_start, &QPushButton::clicked, this, &SocketStreamExtensionForm::onStartPressed);
	connect(this->ui->pushButton_stop, &QPushButton::clicked, this, &SocketStreamExtensionForm::onStopPressed);
}

SocketStreamExtensionForm::~SocketStreamExtensionForm()
{
	delete this->ui;
}

void SocketStreamExtensionForm::setSettings(QVariantMap settings) {
	this->ui->lineEdit_ip->setText(settings.value(HOST_IP).toString());
	this->ui->lineEdit_port->setText(settings.value(HOST_PORT).toString());
	this->ui->lineEdit_pipeName->setText(settings.value(PIPE_NAME).toString());
}

void SocketStreamExtensionForm::getSettings(QVariantMap* settings) {
	settings->insert(HOST_IP, this->parameters.ip);
	settings->insert(HOST_PORT, this->parameters.port);
	settings->insert(PIPE_NAME, this->parameters.pipeName);
}

void SocketStreamExtensionForm::updateParams() {
	this->parameters.ip = this->ui->lineEdit_ip->text();
	this->parameters.port = this->ui->lineEdit_port->text().toInt();
	this->parameters.pipeName = this->ui->lineEdit_pipeName->text();
	this->parameters.mode = this->fromInt(ui->comboBox_mode->currentData().toInt());

	emit paramsChanged(this->parameters);
}

void SocketStreamExtensionForm::onStartPressed() {
	this->updateParams(); //todo: check if this is necessary here
	emit startPressed();
}

void SocketStreamExtensionForm::onStopPressed() {
	emit stopPressed();
}

void SocketStreamExtensionForm::enableButtonsForBroadcastingEnabledState(bool braodcastingActive) {
	bool isTcpIp = ui->comboBox_mode->currentData().value<int>() == this->toInt(CommunicationMode::TCPIP);
	bool isActive = braodcastingActive;

	ui->pushButton_start->setEnabled(!isActive);
	ui->pushButton_stop->setEnabled(isActive);

	//enable or disable IP and Port fields based on TCP/IP mode and broadcasting state
	ui->lineEdit_ip->setEnabled(!isActive && isTcpIp);
	ui->lineEdit_port->setEnabled(!isActive && isTcpIp);

	//enable or disable Pipe Name field based on IPC mode and broadcasting state
	ui->lineEdit_pipeName->setEnabled(!isActive && !isTcpIp);
}

void SocketStreamExtensionForm::findGuiElements(){
	this->checkBoxes = this->findChildren<QCheckBox*>();
	this->doubleSpinBoxes = this->findChildren<QDoubleSpinBox*>();
	this->spinBoxes = this->findChildren<QSpinBox*>();
	this->comboBoxes = this->findChildren<QComboBox*>();
	this->radioButtons = this->findChildren<QRadioButton*>();
	this->lineEdits = this->findChildren<QLineEdit*>();

}

void SocketStreamExtensionForm::connectGuiElementsToUpdateParams() {
	foreach(QCheckBox* widget, this->checkBoxes){
		connect(widget, &QCheckBox::stateChanged, this, &SocketStreamExtensionForm::updateParams);
	}
	foreach(QDoubleSpinBox* widget, this->doubleSpinBoxes){
		connect(widget, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &SocketStreamExtensionForm::updateParams);
	}
	foreach(QSpinBox* widget, this->spinBoxes){
		connect(widget, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SocketStreamExtensionForm::updateParams);
	}
	foreach(QComboBox* widget, this->comboBoxes){
		connect(widget, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SocketStreamExtensionForm::updateParams);
	}
	foreach(QRadioButton* widget, this->radioButtons){
		connect(widget, &QRadioButton::toggled, this, &SocketStreamExtensionForm::updateParams);
	}
	foreach(QLineEdit* widget, this->lineEdits){
		connect(widget, &QLineEdit::editingFinished, this, &SocketStreamExtensionForm::updateParams);
	}
}

void SocketStreamExtensionForm::initValidators() {
	QString ipRange = "(([ 0]+)|([ 0]*[0-9] *)|([0-9][0-9] )|([ 0][0-9][0-9])|(1[0-9][0-9])|([2][0-4][0-9])|(25[0-5]))";
	QRegExp ipRegex("^" + ipRange
					+ "\\." + ipRange
					+ "\\." + ipRange
					+ "\\." + ipRange + "$");
	QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
	this->ui->lineEdit_ip->setValidator(ipValidator);
	//this->ui->lineEdit_ip->setInputMask("000.000.000.000");
	//this->ui->lineEdit_ip->setText("000.000.000.000");

	this->ui->lineEdit_port->setValidator(new QIntValidator(0, 65353, this));
}

void SocketStreamExtensionForm::updateGuiAccordingConnectionMode() {
	bool isTcpIp = ui->comboBox_mode->currentIndex() == 0;
	this->ui->lineEdit_ip->setEnabled(isTcpIp);
	this->ui->lineEdit_port->setEnabled(isTcpIp);
	this->ui->lineEdit_pipeName->setEnabled(!isTcpIp);
}

int SocketStreamExtensionForm::toInt(CommunicationMode mode) {
	return static_cast<int>(mode);
}

CommunicationMode SocketStreamExtensionForm::fromInt(int mode) {
	switch(mode) {
		case static_cast<int>(CommunicationMode::TCPIP):
			return CommunicationMode::TCPIP;
		case static_cast<int>(CommunicationMode::IPC):
			return CommunicationMode::IPC;
		default:
			emit error("Invalid mode value for CommunicationMode enum.");
			return CommunicationMode::TCPIP;
	}
}
