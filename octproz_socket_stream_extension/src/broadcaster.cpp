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


Broadcaster::Broadcaster(QObject *parent) : QObject(parent), tcpServer(nullptr), localServer(nullptr), socket(nullptr), tag("[Socket Stream Extension] - "), isBroadcasting(false) {
}

Broadcaster::~Broadcaster() {
	this->stopBroadcasting();
}

void Broadcaster::configure(const SocketStreamExtensionParameters params) {
	this->stopBroadcasting();
	if (this->tcpServer) {
		delete this->tcpServer;
		this->tcpServer = nullptr;
	}
	if (this->localServer) {
		delete this->localServer;
		this->localServer = nullptr;
	}

	this->params = params;
	if (params.mode == CommunicationMode::TCPIP) {
		this->tcpServer = new QTcpServer(this);
		connect(this->tcpServer, &QTcpServer::newConnection, this, &Broadcaster::onClientConnected);
	} else {
		this->localServer = new QLocalServer(this);
		connect(this->localServer, &QLocalServer::newConnection, this, &Broadcaster::onClientConnected);
	}
}


void Broadcaster::startBroadcasting() {
	this->configure(this->params);
	if (this->params.mode == CommunicationMode::TCPIP && tcpServer) {
		if (!this->tcpServer->listen(QHostAddress(this->params.ip), this->params.port)) {
			emit error(this->tcpServer->errorString());
			return;
		}
	} else if (this->params.mode == CommunicationMode::IPC && this->localServer) {
		if (!this->localServer->listen(this->params.pipeName)) {
			emit error(this->localServer->errorString());
			return;
		}
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

	if(this->tcpServer && this->tcpServer->isListening()){
		this->tcpServer->close();
		this->isBroadcasting = false;
	}
	if(this->localServer && this->localServer->isListening()){
		this->localServer->close();
		this->isBroadcasting = false;
	}
	if(!this->isBroadcasting){
		emit info(this->tag + tr("Broadcasting stopped!"));
		emit listeningEnabled(false);
	}
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
		} else if (this->params.mode == CommunicationMode::IPC) {
			connect(static_cast<QLocalSocket*>(newConnection), &QLocalSocket::disconnected, this, &Broadcaster::onClientDisconnected);
		}
		dataConnections.append(newConnection);
		emit info(this->tag + tr("Client connected!"));
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

void Broadcaster::readyRead() {
	QIODevice* senderDevice = qobject_cast<QIODevice*>(sender());
	if (!senderDevice) return;

	QByteArray data = senderDevice->readAll();

	//handle incoming data
	QString dataString = QString::fromUtf8(data).trimmed();

	if (dataString == "ping") {
		senderDevice->write("pong\n");
		} else if (dataString == "enable_command_only_mode") {
			// Move senderDevice to dataConnections
			if (dataConnections.contains(senderDevice)) {
				dataConnections.removeAll(senderDevice);
				commandConnections.append(senderDevice);
				senderDevice->write("Command mode enabled.\n");
			}
		} else if (dataString == "disable_command_only_mode") {
			// Move senderDevice to commandConnections
			if (commandConnections.contains(senderDevice)) {
				commandConnections.removeAll(senderDevice);
				dataConnections.append(senderDevice);
				senderDevice->write("Command mode disabled.\n");
			}
		} else {
		emit remoteCommandReceived(dataString);
	}
}

void Broadcaster::broadcast(void *buffer, size_t bufferSizeInBytes) {
	for(QIODevice* connection : qAsConst(this->dataConnections)){
		if(connection->isOpen()){
			connection->write(static_cast<const char*>(buffer), bufferSizeInBytes);
		}
	}
}
