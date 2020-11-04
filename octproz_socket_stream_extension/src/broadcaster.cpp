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

#include "broadcaster.h"

Broadcaster::Broadcaster(QObject *parent) : QObject(parent)
{
	this->server = new QTcpServer(this);
	this->tag = "[Socket Stream Extension] - ";
	this->socket = nullptr;
	this->port = DEFAULT_PORT;
	this->hostAddress = new QHostAddress(QHostAddress::LocalHost);

	connect(this->server, &QTcpServer::newConnection, this, &Broadcaster::onClientConnected);
}

Broadcaster::~Broadcaster()
{
	this->stopBroadcasting();
	delete this->hostAddress;
}

void Broadcaster::onClientConnected() {
	this->socket = this->server->nextPendingConnection();
	connect(this->socket, &QIODevice::readyRead, this, &Broadcaster::readyRead);
	connect(this->socket, &QTcpSocket::disconnected, this, &Broadcaster::onClientDisconnected);
	emit info(this->tag + tr("Client connected!"));
}

void Broadcaster::onClientDisconnected() {
	emit info(this->tag + tr("Client disconnected!"));
}

void Broadcaster::readyRead() {
	emit info(this->tag + tr("Received ") + QString::number(this->socket->bytesAvailable()) + tr(" bytes from client."));
	QByteArray input = socket->readAll();
	//todo: handle incomming data from client
}

void Broadcaster::startBroadcasting() {
	if(this->server->isListening()){
		emit info(tr("Server is already listening."));
		return;
	}
	if(this->server->listen(*this->hostAddress, this->port)){
		emit info(this->tag + tr("Sever started listening on ip ") + this->server->serverAddress().toString() + tr(" and port ") + QString::number(this->server->serverPort()));
		emit listeningEnabled(true);
	}else{
		QString err = this->server->errorString();
		emit error(this->tag + tr("Unable to listen on ip ") + this->hostAddress->toString() + tr(" and port ") + QString::number(this->port) + tr(". ") + this->server->errorString());
		this->listeningEnabled(false);
	}
}

void Broadcaster::stopBroadcasting() {
	if(this->server->isListening()){
		this->server->close();
		this->socket = nullptr;
		emit listeningEnabled(false);
		emit info(this->tag + tr("Server stopped listening."));
	}
}

void Broadcaster::setHostAddress(QString host){
	this->hostAddress->setAddress(host);
}

void Broadcaster::setPort(quint16 port) {
	this->port = port;
}

void Broadcaster::broadcast(void *buffer, size_t bufferSizeInBytes) {
	if(this->socket != nullptr){
		this->socket->write(static_cast<const char*>(buffer), bufferSizeInBytes);
	}
}
