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

#ifndef BROADCASTER_H
#define BROADCASTER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QByteArray>
#include "socketstreamextensionparameters.h"

class Broadcaster : public QObject {
	Q_OBJECT

public:
	explicit Broadcaster(QObject *parent = nullptr);
	~Broadcaster();

signals:
	void listeningEnabled(bool enabled);
	void error(const QString message);
	void info(const QString message);
	void remoteCommandReceived(const QString command);

public slots:
	void startBroadcasting();
	void stopBroadcasting();
	void setParams(const SocketStreamExtensionParameters params);
	void configure(const SocketStreamExtensionParameters params);
	void onClientConnected();
	void onWebSocketConnected();
	void onWebSocketDisconnected();
	void onClientDisconnected();
	void onBinaryMessageReceived(QByteArray message); // slot to handle binary WebSocket messages
	void onTextMessageReceived(QString message); // slot to handle text WebSocket messages
	void readyRead(); // slot to handle incoming data from the client
	void broadcast(void *buffer, quint32 bufferSizeInBytes, quint16 framesPerBuffer, quint16 frameWidth, quint16 frameHeight, quint8 bitDepth);

private:
	QTcpServer *tcpServer;
	QLocalServer *localServer;
	QWebSocketServer *webSocketServer;
	QList<QTcpSocket*> tcpConnections;
	QList<QLocalSocket*> localConnections;
	QList<QWebSocket*> webSocketConnections;
	SocketStreamExtensionParameters params;
	QString tag;
	bool isBroadcasting;

	QList<QIODevice*> commandConnections; // for clients that only want to send commands
	QList<QIODevice*> dataConnections; // for clients primarily receiving OCT data, with command sending allowed.

	static quint32 startIdentifier;
};

#endif // BROADCASTER_H
