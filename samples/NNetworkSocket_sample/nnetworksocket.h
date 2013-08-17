/*
    nnetworksocket.h - Nyanko Network Socket for Qt declaration.
    
    Revision 0
    
    Features:
            - 
    
    Notes:
            -
            
    2013/07/28, Maya Posch
    (c) Nyanko.ws
*/


#ifndef NNETWORKSOCKET_H
#define NNETWORKSOCKET_H

#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include <QObject>
#include <QIODevice>
#include <QAbstractSocket>

enum SocketState {
    UnconnectedState = 0,   // socket is not connected.
    HostLookupState = 1,    // socket is performing a host name lookup.
    ConnectingState = 2,    // socket has started establishing a connection.
    ConnectedState = 3,     // A connection is established.
    BoundState = 4,         // socket is bound to an address and port.
    ClosingState = 6,       // socket is about to close (data may still be waiting).
    ListeningState = 5      // For internal use only.
};


enum SocketFamily {    
    SOCKET_FAMILY_UNSPEC = 0,   // The address family is unspecified.
    SOCKET_FAMILY_INET = 2,     // Internet Protocol version 4 (IPv4).
    SOCKET_FAMILY_NETBIOS = 17, // The NetBIOS address family.
    SOCKET_FAMILY_INET6 = 23,   // Internet Protocol version 6 (IPv6).
    SOCKET_FAMILY_BTH = 32      // The Bluetooth address family.
};


enum SocketType {    
    SOCKET_TYPE_STREAM = 1,    // Uses TCP for the Internet address family.
    SOCKET_TYPE_DGRAM = 2,     // Uses UDP for the Internet address family.
    SOCKET_TYPE_RAW = 3,       // Provides a raw socket.
    SOCKET_TYPE_RDM = 4,       // Provides a reliable message datagram.
    SOCKET_TYPE_SEQPACKET = 5  // Provides a pseudo-stream packet based on datagrams.
};


class NNetworkSocket : public QObject {
    Q_OBJECT
public:
    explicit NNetworkSocket(QObject *parent = 0);
    ~NNetworkSocket();
    
    bool setProperties(QString host, int port, int family, int type);
    bool connect();
    bool setSocketDescriptor(qintptr socketDescriptor, 
                             SocketState socketState = ConnectedState, 
                             QIODevice::OpenMode openMode = QIODevice::ReadWrite);
    qintptr socketDescriptor() const;
    int read(QByteArray &buf, int count);
    bool read(QByteArray &buf);
    bool write(QByteArray &buf);
    QString errorString();
    bool isOpen() const;
    QAbstractSocket::SocketError error();
    void close();
    
signals:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError err);
    
public slots:
    
private:
    bool open;
    int errorValue;
#ifdef WIN32
    SOCKET socketDesc;
#else
    int socketDesc;
#endif
    SocketState socketState;
    QIODevice::OpenMode openMode;
    struct addrinfo* res;  // will point to the getaddrinfo() results
};

#endif // NNETWORKSOCKET_H
