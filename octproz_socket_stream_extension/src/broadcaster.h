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
