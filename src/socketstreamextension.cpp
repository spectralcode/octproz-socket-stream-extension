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
#include <math.h>
#include <QtGlobal>

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

	this->autoConnect();
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
	QMetaObject::invokeMethod(this->broadcastServer, "setParams", Qt::QueuedConnection, Q_ARG(SocketStreamExtensionParameters, params));
	this->storeParameters();
}

void SocketStreamExtension::storeParameters() {
	//update settingsMap, so parameters can be reloaded into gui at next start of application
	this->form->getSettings(&this->settingsMap);
	emit storeSettings(this->name, this->settingsMap);
}

void SocketStreamExtension::handleRemoteCommand(QString command) {
	command = command.trimmed();

	if (command.compare("remote_start", Qt::CaseInsensitive) == 0) {
		emit startProcessingRequest();
	}
	else if (command.compare("remote_stop", Qt::CaseInsensitive) == 0) {
		emit stopProcessingRequest();
	}
	else if (command.compare("remote_record", Qt::CaseInsensitive) == 0) {
		emit startRecordingRequest();
	}
	else if (command.startsWith("load_settings", Qt::CaseInsensitive)) {
		this->handleSettingsCommand(command, "load");
	}
	else if (command.startsWith("save_settings", Qt::CaseInsensitive)) {
		this->handleSettingsCommand(command, "save");
	}
	else if(command.startsWith("remote_plugin_control", Qt::CaseInsensitive)){
		this->handleRemotePluginControlCommand(command);
	}
	else if(command.startsWith("set_disp_coeff")) {
		this->handleSetDispCoeffCommand(command);
	}
	else if (command.startsWith("set_grayscale_conversion", Qt::CaseInsensitive)) {
		this->handleSetGrayscaleConversionCommand(command);
	}
	else {
		emit error("Unknown command: " + command);
	}
	//emit info("Remote command received: " + command);
}

void SocketStreamExtension::handleSettingsCommand(const QString& command, const QString& action) {
	QStringList parts = command.split(":", QString::SkipEmptyParts);
	if (parts.size() >= 2) {
		QString fileName = parts.mid(1).join(":").trimmed();
		if (action == "load") {
			emit loadSettingsFileRequest(fileName);
		} else if (action == "save") {
			emit saveSettingsFileRequest(fileName);
		}
	} else {
		emit error(QString("Invalid %1_settings command format: %2").arg(action, command));
	}
}

void SocketStreamExtension::handleRemotePluginControlCommand(const QString &command) {
	//Expected format: remote_plugin_control, PluginName, Command
	QStringList parts = command.split(",", QString::SkipEmptyParts);
	if(parts.size() < 3){
		emit error("Invalid remote_plugin_control command format: " + command);
		return;
	}
	QString targetPlugin = parts.at(1).trimmed();
	QString pluginCommand = parts.mid(2).join(",").trimmed();
	emit sendCommand(this->getName(), targetPlugin, pluginCommand);
	emit info("Sent plugin command '" + pluginCommand + "' to plugin '" + targetPlugin + "'");
}

void SocketStreamExtension::handleSetDispCoeffCommand(const QString& command) {
		double coeffs[4];
		double* results[4];
		bool conversionSuccessful[4];
		bool success = true;

		QStringList parts = command.toLower().split(":", QString::SkipEmptyParts);
		if (parts.size() == 5) {
		parts.removeFirst();
			for(int i = 0; i < 4; i++){
				double val = parts[i].toDouble(&(conversionSuccessful[i]));
				if(conversionSuccessful[i]){
					coeffs[i] = val;
					results[i] = &(coeffs[i]);
				}
				 else if(parts[i] == "nullptr" || parts[i] == "null") {
					results[i] = nullptr;
					conversionSuccessful[i] = true;
				 }
				 success = success && conversionSuccessful[i];
			}
			if (success) {
				emit setDispCompCoeffsRequest(results[0], results[1], results[2], results[3]);
			} else {
				emit error("One or more coefficients could not be converted to a double.");
			}
		} else {
			emit error("Invalid dispersion coefficients command");
		}
}

void SocketStreamExtension::handleSetGrayscaleConversionCommand(const QString& command) {
	QStringList parts = command.toLower().split(":", QString::SkipEmptyParts);
	if (parts.size() == 6) {
		bool ok[5];
		bool enableLogScaling;

		if (parts[1] == "true" || parts[1] == "1") {
			enableLogScaling = true;
			ok[0] = true;
		} else if (parts[1] == "false" || parts[1] == "0") {
			enableLogScaling = false;
			ok[0] = true;
		} else {
			ok[0] = false;
		}

		auto parseDouble = [](const QString& str, bool& ok) -> double {
			QString trimmedStr = str.trimmed();
			if (trimmedStr == "nan" || trimmedStr == "null" || trimmedStr == "nullptr") {
				ok = true;
				return qQNaN();
			}
			return trimmedStr.toDouble(&ok);
		};

		double max = parseDouble(parts[2], ok[1]);
		double min = parseDouble(parts[3], ok[2]);
		double multiplicator = parseDouble(parts[4], ok[3]);
		double offset = parseDouble(parts[5], ok[4]);

		bool success = ok[0] && ok[1] && ok[2] && ok[3] && ok[4];

		if (success) {
			emit setGrayscaleConversionRequest(enableLogScaling, max, min, multiplicator, offset);
		} else {
			emit error("One or more coefficients could not be converted to the expected types.");
		}
	} else {
		emit error("Invalid grayscale conversion command: " + command);
	}
}

void SocketStreamExtension::autoConnect() {
	//get current settings from the form
	QVariantMap currentSettings;
	this->form->getSettings(&currentSettings);
	bool autoConnect = currentSettings.value(AUTO_CONNECT_ENABLED, false).toBool();
	QString ip = currentSettings.value(HOST_IP).toString();

	//if auto connect is enabled and an IP address is provided, start broadcasting automatically
	if (autoConnect && !ip.isEmpty()) {
		emit info("Auto connecting to socket stream on startup...");
		QMetaObject::invokeMethod(this->broadcastServer,
			"startBroadcasting",
			Qt::QueuedConnection);
	}
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

		// Calculate bytes per sample
		size_t bytesPerSample = ceil(static_cast<double>(bitDepth) / 8.0);

		// Calculate total buffer size in bytes
		size_t bufferSizeInBytes = samplesPerLine * linesPerFrame * framesPerBuffer * bytesPerSample;

		// Cast bufferSizeInBytes to quint32 (ensure it does not exceed quint32 limits)
		quint32 quintBufferSizeInBytes = static_cast<quint32>(bufferSizeInBytes);

		// Cast other parameters to required types
		quint16 quintFramesPerBuffer = static_cast<quint16>(framesPerBuffer);
		quint16 quintFrameWidth = static_cast<quint16>(samplesPerLine);
		quint16 quintFrameHeight = static_cast<quint16>(linesPerFrame);
		quint8 quintBitDepth = static_cast<quint8>(bitDepth);

		// Invoke the broadcast method with all required parameters
		QMetaObject::invokeMethod(this->broadcastServer, "broadcast", Qt::QueuedConnection,
								  Q_ARG(void*, buffer),
								  Q_ARG(quint32, quintBufferSizeInBytes),
								  Q_ARG(quint16, quintFramesPerBuffer),
								  Q_ARG(quint16, quintFrameWidth),
								  Q_ARG(quint16, quintFrameHeight),
								  Q_ARG(quint8, quintBitDepth));
	}
}
