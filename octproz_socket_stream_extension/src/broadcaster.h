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

#ifndef BROADCASTER_H
#define BROADCASTER_H

#define DEFAULT_PORT 1234

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class Broadcaster : public QObject
{
	Q_OBJECT
public:
	explicit Broadcaster(QObject *parent = nullptr);
	~Broadcaster();

private:
	QTcpServer* server;
	QTcpSocket* socket;
	QString tag;
	qint16 port;
	QHostAddress* hostAddress;

public slots:
	void onClientConnected();
	void onClientDisconnected();
	void readyRead();
	void startBroadcasting();
	void stopBroadcasting();
	void setHostAddress(QString host);
	void setPort(quint16 port);

	void broadcast(void* buffer, size_t bufferSizeInBytes);

	//void fillBuffers(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr);


signals:
	void info(QString);
	void error(QString);
	void listeningEnabled(bool);

};

#endif // BROADCASTER_H
