#ifndef SOCKETSTREAMEXTENSIONPARAMETERS_H
#define SOCKETSTREAMEXTENSIONPARAMETERS_H

#include <QString>
#include <QtGlobal>
#include <QMetaType>

// Enum to choose between
// inter-process communication (IPC) --> QLocalSockets
// and TCP/IP communication --> QTcpSocket
enum class CommunicationMode {
	IPC,
	TCPIP,
	WebSocket
};

struct SocketStreamExtensionParameters {
	CommunicationMode mode;
	QString pipeName;
	QString ip;
	quint16 port;
	bool sendHeader;
	bool sendTimestamp;  // append send-side wall-clock ms to header (requires sendHeader)
	bool tcpNoDelay;     // disable Nagle on new TCP connections (TCP mode only)
	bool autoConnect;
};
Q_DECLARE_METATYPE(SocketStreamExtensionParameters)

#endif // SOCKETSTREAMEXTENSIONPARAMETERS_H
