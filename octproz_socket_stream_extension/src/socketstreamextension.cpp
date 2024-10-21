/**
**  This file is part of SocketStreamExtension for OCTproZ.
**  Copyright (C) 2020,2024 Miroslav Zabic
**
**  SocketStreamExtension is an OCTproZ extension designed for streaming
**  processed OCT data, supporting inter-process communication via local
**  socket connections (using Unix Domain Sockets on Unix/Linux and Named
**  Pipes on Windows) and network communication across computers via TCP/IP.
**  This enables OCT image data streaming to different applications on the
**  same computer or to different computers on the same network.
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
**			spectralcode.de
****
**/

#include "socketstreamextension.h"


SocketStreamExtension::SocketStreamExtension() : Extension() {
	qRegisterMetaType<QVector<qreal> >("QVector<qreal>");
	qRegisterMetaType<SocketStreamExtensionParameters>("SocketStreamExtensionParameters");
	qRegisterMetaType<CommunicationMode>("CommunicationMode");

	//init extension
	this->setType(EXTENSION);
	this->displayStyle = SIDEBAR_TAB;
	this->name = "Socket Stream Extension";
	this->toolTip = "Stream OCT data over sockets";
	this->active = false;

	//init gui
	this->form = new SocketStreamExtensionForm();
	this->widgetDisplayed = false;
	connect(this->form, &SocketStreamExtensionForm::paramsChanged, this, &SocketStreamExtension::setParams);

	//setup broadcaster gui connections and move broadcaster to thread
	this->broadcastServer = new Broadcaster();
	this->broadcastServer->moveToThread(&broadcasterThread);
	connect(this->broadcastServer, &Broadcaster::info, this, &SocketStreamExtension::info);
	connect(this->broadcastServer, &Broadcaster::error, this, &SocketStreamExtension::error);
	connect(this->broadcastServer, &Broadcaster::listeningEnabled, this->form, &SocketStreamExtensionForm::enableButtonsForBroadcastingEnabledState);
	connect(this->form, &SocketStreamExtensionForm::startPressed, this->broadcastServer, &Broadcaster::startBroadcasting);
	connect(this->form, &SocketStreamExtensionForm::stopPressed, this->broadcastServer, &Broadcaster::stopBroadcasting);
	connect(this->broadcastServer, &Broadcaster::remoteCommandReceived, this, &SocketStreamExtension::handleRemoteCommand);
	connect(&broadcasterThread, &QThread::finished, this->broadcastServer, &Broadcaster::deleteLater);
	broadcasterThread.start();
}

SocketStreamExtension::~SocketStreamExtension() {
	broadcasterThread.quit();
	broadcasterThread.wait();

	if(!this->widgetDisplayed){
		delete this->form;
	}
}

QWidget* SocketStreamExtension::getWidget() {
	this->widgetDisplayed = true;
	return this->form;
}

void SocketStreamExtension::activateExtension() {
	//this method is called by OCTproZ as soon as user activates the extension. If the extension controls hardware components, they can be prepared, activated, initialized or started here.
	this->active = true;
}

void SocketStreamExtension::deactivateExtension() {
	//this method is called by OCTproZ as soon as user deactivates the extension. If the extension controls hardware components, they can be deactivated, resetted or stopped here.
	this->active = false;
}

void SocketStreamExtension::settingsLoaded(QVariantMap settings) {
	//this method is called by OCTproZ and provides a QVariantMap with stored settings/parameters.
	this->form->setSettings(settings); //update gui with stored settings
}

void SocketStreamExtension::setParams(SocketStreamExtensionParameters params) {
	this->params = params;
	QMetaObject::invokeMethod(this->broadcastServer, "configure", Qt::QueuedConnection, Q_ARG(SocketStreamExtensionParameters, params));
	this->storeParameters();
}

void SocketStreamExtension::storeParameters() {
	//update settingsMap, so parameters can be reloaded into gui at next start of application
	this->form->getSettings(&this->settingsMap);
	emit storeSettings(this->name, this->settingsMap);
}

void SocketStreamExtension::handleRemoteCommand(QString command) {
	command = command.toLower();
	if(command == "remote_start") {
		emit startProcessingRequest();
	}
	else if(command == "remote_stop") {
		emit stopProcessingRequest();
	}
	else if(command == "remote_record") {
		emit startRecordingRequest();
	}
	else if (command.startsWith("load_settings", Qt::CaseInsensitive)) {
		QStringList parts = command.split(":", QString::SkipEmptyParts);
		if (parts.size() >= 2) {
			QString fileName = parts.mid(1).join(":").trimmed();
			emit loadSettingsFileRequest(fileName);
		} else {
			emit error("Invalid load_settings command format.");
		}
	}
	else if (command.startsWith("save_settings", Qt::CaseInsensitive)) {
		QStringList parts = command.split(":", QString::SkipEmptyParts);
		if (parts.size() >= 2) {
			QString fileName = parts.mid(1).join(":").trimmed();
			emit saveSettingsFileRequest(fileName);
		} else {
			emit error("Invalid save_settings command format.");
		}
	}
	emit info("Remote command received: " + command);
}

void SocketStreamExtension::rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	//do nothing here as we do not need the raw data. Q_UNUSED is used to suppress compiler warnings
	Q_UNUSED(buffer)
	Q_UNUSED(bitDepth)
	Q_UNUSED(samplesPerLine)
	Q_UNUSED(linesPerFrame)
	Q_UNUSED(framesPerBuffer)
	Q_UNUSED(buffersPerVolume)
	Q_UNUSED(currentBufferNr)
}

void SocketStreamExtension::processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->active){
		Q_UNUSED(buffersPerVolume)
		Q_UNUSED(currentBufferNr)

		size_t bytesPerSample = ceil(static_cast<double>(bitDepth) / 8.0);
		size_t bufferSizeInBytes = samplesPerLine * linesPerFrame * framesPerBuffer * bytesPerSample;
		QMetaObject::invokeMethod(this->broadcastServer, "broadcast", Qt::QueuedConnection, Q_ARG(void*, buffer), Q_ARG(size_t, bufferSizeInBytes));
	}
}

