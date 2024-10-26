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

#include "broadcaster.h"
#include "socketstreamextensionparameters.h"
#include <QHostAddress>
#include <QDataStream>
#include <QDebug>

quint32 Broadcaster::startIdentifier = 299792458; // identifier (magic number) for synchronization on client side

Broadcaster::Broadcaster(QObject *parent) : QObject(parent), tcpServer(nullptr), localServer(nullptr), webSocketServer(nullptr), tag("[Socket Stream Extension] - "), isBroadcasting(false) {
}

Broadcaster::~Broadcaster() {
	this->stopBroadcasting();
}

void Broadcaster::configure(const SocketStreamExtensionParameters params) {
	this->stopBroadcasting();
	if (this->tcpServer) {
		this->tcpServer->deleteLater();
		this->tcpServer = nullptr;
	}
	if (this->localServer) {
		this->localServer->deleteLater();
		this->localServer = nullptr;
	}
	if (this->webSocketServer) {
		this->webSocketServer->deleteLater();
		this->webSocketServer = nullptr;
	}

	this->params = params;
	switch (this->params.mode) {
		case CommunicationMode::TCPIP:
			this->tcpServer = new QTcpServer(this);
			connect(this->tcpServer, &QTcpServer::newConnection, this, &Broadcaster::onClientConnected);
			break;
		case CommunicationMode::IPC:
			this->localServer = new QLocalServer(this);
			connect(this->localServer, &QLocalServer::newConnection, this, &Broadcaster::onClientConnected);
			break;
		case CommunicationMode::WebSocket:
			this->webSocketServer = new QWebSocketServer(QStringLiteral("Broadcaster WebSocket Server"), QWebSocketServer::NonSecureMode, this);
			connect(this->webSocketServer, &QWebSocketServer::newConnection, this, &Broadcaster::onWebSocketConnected);
			break;
		default:
			qWarning() << "Unknown Communication Mode!";
			break;
	}
}

void Broadcaster::startBroadcasting() {
	this->configure(this->params);

	switch (this->params.mode) {
		case CommunicationMode::TCPIP:
			if (tcpServer) {
				if (!this->tcpServer->listen(QHostAddress(this->params.ip), this->params.port)) {
					emit error(this->tcpServer->errorString());
					return;
				}
			}
			break;
		case CommunicationMode::IPC:
			if (localServer) {
				if (!this->localServer->listen(this->params.pipeName)) {
					emit error(this->localServer->errorString());
					return;
				}
			}
			break;
		case CommunicationMode::WebSocket:
			if (webSocketServer) {
				if (!this->webSocketServer->listen(QHostAddress::Any, this->params.port)) { // WebSocket typically listens on a port
					emit error(this->webSocketServer->errorString());
					return;
				}
			}
			break;
		default:
			qWarning() << "Unknown Communication Mode!";
			return;
	}

	this->isBroadcasting = true;
	emit listeningEnabled(true);
}

void Broadcaster::stopBroadcasting() {
	if(!this->isBroadcasting){
		return;
	}
	for (QIODevice* connection : qAsConst(this->commandConnections)) {
		connection->close();
		connection->deleteLater();
	}
	this->commandConnections.clear();

	for (QIODevice* connection : qAsConst(this->dataConnections)) {
		connection->close();
		connection->deleteLater();
	}
	this->dataConnections.clear();

	for (QWebSocket* ws : qAsConst(this->webSocketConnections)) {
		ws->close();
		ws->deleteLater();
	}
	this->webSocketConnections.clear();

	if(this->tcpServer && this->tcpServer->isListening()){
		this->tcpServer->close();
		this->isBroadcasting = false;
	}
	if(this->localServer && this->localServer->isListening()){
		this->localServer->close();
		this->isBroadcasting = false;
	}
	if(this->webSocketServer && this->webSocketServer->isListening()){
		this->webSocketServer->close();
		this->isBroadcasting = false;
	}
	if(!this->isBroadcasting){
		emit info(this->tag + tr("Broadcasting stopped!"));
		emit listeningEnabled(false);
	}
}

void Broadcaster::setParams(const SocketStreamExtensionParameters params) {
	this->params = params;
}

void Broadcaster::onClientConnected() {
	QIODevice* newConnection = nullptr;
	if (this->params.mode == CommunicationMode::TCPIP && this->tcpServer) {
		newConnection = this->tcpServer->nextPendingConnection();
	} else if (this->params.mode == CommunicationMode::IPC && this->localServer) {
		newConnection = this->localServer->nextPendingConnection();
	}

	if (newConnection) {
		connect(newConnection, &QIODevice::readyRead, this, &Broadcaster::readyRead);
		// Since QIODevice doesn't have a disconnected signal, we cast based on mode
		if (this->params.mode == CommunicationMode::TCPIP) {
			connect(static_cast<QTcpSocket*>(newConnection), &QTcpSocket::disconnected, this, &Broadcaster::onClientDisconnected);
			tcpConnections.append(static_cast<QTcpSocket*>(newConnection));
		} else if (this->params.mode == CommunicationMode::IPC) {
			connect(static_cast<QLocalSocket*>(newConnection), &QLocalSocket::disconnected, this, &Broadcaster::onClientDisconnected);
			localConnections.append(static_cast<QLocalSocket*>(newConnection));
		}
		dataConnections.append(newConnection);
		emit info(this->tag + tr("Client connected!"));
	}
}

void Broadcaster::onWebSocketConnected() {
	if (!webSocketServer) return;

	QWebSocket *client = webSocketServer->nextPendingConnection();

	connect(client, &QWebSocket::binaryMessageReceived, this, &Broadcaster::onBinaryMessageReceived);
	connect(client, &QWebSocket::textMessageReceived, this, &Broadcaster::onTextMessageReceived);
	connect(client, &QWebSocket::disconnected, this, &Broadcaster::onWebSocketDisconnected);

	webSocketConnections.append(client);

	emit info(this->tag + tr("WebSocket client connected!"));
}

void Broadcaster::onWebSocketDisconnected() {
	QWebSocket* disconnectedClient = qobject_cast<QWebSocket*>(sender());
	if (disconnectedClient) {
		webSocketConnections.removeAll(disconnectedClient);
		disconnectedClient->deleteLater();
		emit info(this->tag + tr("WebSocket client disconnected."));
	}
}

void Broadcaster::onClientDisconnected() {
	QIODevice* disconnectedDevice  = qobject_cast<QIODevice*>(sender());
	if(disconnectedDevice ){
		this->commandConnections.removeAll(disconnectedDevice );
		this->dataConnections.removeAll(disconnectedDevice );
		disconnectedDevice ->deleteLater();
		emit info(this->tag + tr("Client disconnected."));
	}
}

void Broadcaster::onBinaryMessageReceived(QByteArray message) {
	// Handle binary messages from WebSocket clients
	// Hier gehen wir davon aus, dass binäre Nachrichten Befehle oder ähnliche Daten enthalten
	QString dataString = QString::fromUtf8(message).trimmed();

	if (dataString == "ping") {
		QWebSocket* webSocket = qobject_cast<QWebSocket*>(sender());
		if (webSocket) {
			webSocket->sendTextMessage("pong\n");
		}
	} else if (dataString == "enable_command_only_mode") {
		// Move webSocket to commandConnections
		QWebSocket* webSocket = qobject_cast<QWebSocket*>(sender());
		if (webSocket && webSocketConnections.contains(webSocket)) {
			webSocketConnections.removeAll(webSocket);
			commandConnections.append(reinterpret_cast<QIODevice*>(webSocket));
			webSocket->sendTextMessage("Command mode enabled.\n");
		}
	} else if (dataString == "disable_command_only_mode") {
		// Move webSocket to dataConnections
		QWebSocket* webSocket = qobject_cast<QWebSocket*>(sender());
		if (webSocket && commandConnections.contains(reinterpret_cast<QIODevice*>(webSocket))) {
			commandConnections.removeAll(reinterpret_cast<QIODevice*>(webSocket));
			webSocketConnections.append(webSocket);
			webSocket->sendTextMessage("Command mode disabled.\n");
		}
	} else {
		emit remoteCommandReceived(dataString);
	}
}

void Broadcaster::onTextMessageReceived(QString message) {
	// Handle text messages from WebSocket clients
	QString dataString = message.trimmed();

	if (dataString == "ping") {
		QWebSocket* webSocket = qobject_cast<QWebSocket*>(sender());
		if (webSocket) {
			webSocket->sendTextMessage("pong\n");
		}
	} else if (dataString == "enable_command_only_mode") {
		// Move webSocket to commandConnections
		QWebSocket* webSocket = qobject_cast<QWebSocket*>(sender());
		if (webSocket && webSocketConnections.contains(webSocket)) {
			webSocketConnections.removeAll(webSocket);
			commandConnections.append(reinterpret_cast<QIODevice*>(webSocket));
			webSocket->sendTextMessage("Command mode enabled.\n");
		}
	} else if (dataString == "disable_command_only_mode") {
		// Move webSocket to dataConnections
		QWebSocket* webSocket = qobject_cast<QWebSocket*>(sender());
		if (webSocket && commandConnections.contains(reinterpret_cast<QIODevice*>(webSocket))) {
			commandConnections.removeAll(reinterpret_cast<QIODevice*>(webSocket));
			webSocketConnections.append(webSocket);
			webSocket->sendTextMessage("Command mode disabled.\n");
		}
	} else {
		emit remoteCommandReceived(dataString);
	}
}

void Broadcaster::readyRead() {
	QIODevice* senderDevice = qobject_cast<QIODevice*>(sender());
	if (!senderDevice) return;

	QByteArray data = senderDevice->readAll();

	//handle incoming data
	QString dataString = QString::fromUtf8(data).trimmed();

	if (dataString == "ping") {
		senderDevice->write("pong\n");
	} else if (dataString == "enable_command_only_mode") {
		// Move senderDevice to commandConnections
		if (dataConnections.contains(senderDevice)) {
			dataConnections.removeAll(senderDevice);
			commandConnections.append(senderDevice);
			senderDevice->write("Command mode enabled.\n");
		}
	} else if (dataString == "disable_command_only_mode") {
		// Move senderDevice to dataConnections
		if (commandConnections.contains(senderDevice)) {
			commandConnections.removeAll(senderDevice);
			dataConnections.append(senderDevice);
			senderDevice->write("Command mode disabled.\n");
		}
	} else {
		emit remoteCommandReceived(dataString);
	}
}

void Broadcaster::broadcast(void *buffer, quint32 bufferSizeInBytes, quint16 framesPerBuffer, quint16 frameWidth, quint16 frameHeight, quint8 bitDepth) {
	QByteArray frameData;
	QDataStream stream(&frameData, QIODevice::WriteOnly);
	stream.setByteOrder(QDataStream::BigEndian);

	// write header information to stream
	if(this->params.sendHeader) {
		stream << startIdentifier << bufferSizeInBytes << frameWidth << frameHeight << bitDepth;
	}

	// append the actual OCT image data
	frameData.append(static_cast<const char*>(buffer), bufferSizeInBytes);

	// send the data to TCP/IP and IPC clients
	for (QIODevice* connection : qAsConst(this->dataConnections)) {
		if (connection->isOpen()) {
			qint64 bytesWritten = connection->write(frameData);
			if (bytesWritten == -1) {
				emit error(this->tag + tr("Failed to write to client: %1").arg(connection->errorString()));
			}
		}
	}

	// send the data to WebSocket clients
	for (QWebSocket* ws : qAsConst(this->webSocketConnections)) {
		if (ws->isValid()) {
			ws->sendBinaryMessage(frameData);
		}
	}
}

