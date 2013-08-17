/*
    nnetworksocket.cpp - Nyanko Network Socket for Qt implementation.
    
    Revision 0
    
    Features:
            - 
    
    Notes:
            -
            
    2013/07/28, Maya Posch
    (c) Nyanko.ws
*/


#include "nnetworksocket.h"


// --- CONSTRUCTOR ---
NNetworkSocket::NNetworkSocket(QObject *parent) : QObject(parent) {
#ifdef WIN32
    WSADATA wsaData;
    int iResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        qDebug() << "WSAStartup failed:" << iResult;
        return;
    }
#endif
    
    //
}


// --- DECONSTRUCTOR ---
NNetworkSocket::~NNetworkSocket() {
    close();
    freeaddrinfo(res); // free the linked-list
    
#ifdef WIN32
    WSACleanup();
#endif
}


// --- SET PROPERTIES ---
// Set the socket properties prior to obtaining a socket handle.
bool NNetworkSocket::setProperties(QString host, int port, int family, int type) {
    int status;
    struct addrinfo hints;
    res = 0;
    
    memset(&hints, 0, sizeof hints);    // make sure the struct is empty
    hints.ai_family = family;       // don't care IPv4 or IPv6
    hints.ai_socktype = type;       // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me
    hints.ai_protocol = IPPROTO_TCP;
    
    qDebug() << "Host: " << host.toLatin1().data();
    qDebug() << "Port: " << QByteArray::number(port).data();
    
    if ((status = getaddrinfo(host.toLatin1().data(), QByteArray::number(port).data(), 
                              &hints, &res)) != 0) {
        qDebug() << "getaddrinfo error:" << gai_strerror(status);
        emit error(QAbstractSocket::NetworkError);
        return false;
    }
    
    socketDesc = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketDesc == -1) {
#ifdef WIN32
        errorValue = WSAGetLastError();
#else
        errorValue = -1;
#endif
        freeaddrinfo(res);
        return false;
    }
    
    return true;
}


// --- CONNECT ---
// Connect on the configured socket to the earlier provided host and port.
bool NNetworkSocket::connect() {
    if (res == 0) { return false; }
    
    qDebug() << "Connect()";
    qDebug() << "Family:" << res->ai_addr->sa_family;
    char ipstr[INET6_ADDRSTRLEN];
    struct addrinfo *p;
    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
    
        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        qDebug() << ipver << ipstr;
    }
    
    int err = ::connect(socketDesc, res->ai_addr, res->ai_addrlen);
    if (err == -1) {
#ifdef WIN32
        errorValue = WSAGetLastError();
#else
        errorValue = -1;
#endif
        return false;
    }
    else {
        open = true;
    }
    
    // Set the time-out for receive actions on the socket.
#ifdef WIN32
    int timeout = 5000; // 5 seconds
    err = setsockopt(socketDesc, SOL_SOCKET, SO_RCVTIMEO, 
                         (const char*) &timeout, sizeof(timeout));
    if (err == -1) {
        errorValue = WSAGetLastError();
        return false;
    }
#else
    struct timeval timeout;    
    timeout.tv_sec = 5; // 5 seconds
    err = setsockopt(socketDesc, SOL_SOCKET, SO_RCVTIMEO, 
                         (struct timeval*) timeout, sizeof(timeout));
    if (err == -1) {
        errorValue = -1;
    }
#endif
    
    return true;
}


// --- SET SOCKET DESCRIPTOR ---
bool NNetworkSocket::setSocketDescriptor(qintptr socketDescriptor, 
                                         SocketState socketState, 
                                         QIODevice::OpenMode openMode) {
    this->socketState = socketState;
    this->openMode = openMode;
    this->open = true;
    
#ifdef WIN32
    this->socketDesc = (SOCKET) socketDescriptor;
    int err = WSAEventSelect(socketDesc, NULL, 0); // reset to enable blocking mode
    int lastError;
    if (err == SOCKET_ERROR) {
        lastError = WSAGetLastError();
        qDebug() << "WSAEventSelect failed with error:" << err;
        qDebug() << "WSAGetLastError():" << lastError;
    }
        
    u_long iMode = 0; // blocking
    int iResult = ioctlsocket(socketDesc, FIONBIO, &iMode);
    lastError = WSAGetLastError();
    if (iResult != NO_ERROR) {
        qDebug() << "ioctlsocket failed with error:" << iResult;
        qDebug() << "WSAGetLastError():" << lastError;
    }
#else
    // POSIX
    this->socketDesc = (int) socketDescriptor;
    
    // turn into blocking socket.
    int opts;
    opts = fcntl(socketDesc, F_GETFL);
    opts ^= O_NONBLOCK;
    fcntl(socketDesc, F_SETFL, opts);
#endif
    qDebug() << "NNetworkSocket: Set socket descriptor:" << this->socketDesc;
    
    emit connected();
    return true;
}


// --- SOCKET DESCRIPTOR ---
// Returns the socket descriptor for this instance.
qintptr NNetworkSocket::socketDescriptor() const {
    return (qintptr) socketDesc;
}


// --- READ ---
// Reads count bytes from the network buffer and returns it in the provided buffer.
// Returns 1 or 2 if successful, else 0 with the error available via error().
// 0 - error.
// 1 - no exceptions.
// 2 - remote disconnect on socket.
int NNetworkSocket::read(QByteArray &buf, int count) {
    if (!open) { return 0; } // false
    char* buffer = new char[count];
    int bytesRead = 0;
    int bytesLeft = count;
    
    // checking for data availability in the buffer
    fd_set fds;
    int ret = 0;
    int waitTimeUs = 10000000; // 10 seconds
    struct timeval timeout;
    struct timeval* timeoutPtr = 0;
    
    if (waitTimeUs > 0) {
        timeout.tv_sec = waitTimeUs / 1000000;
        timeout.tv_usec = waitTimeUs % 1000000;
        timeoutPtr = &timeout;
    }
    
    FD_ZERO(&fds);
    FD_SET(socketDesc, &fds);
    
    ret = select(socketDesc + 1, &fds, 0, 0, timeoutPtr);
    if (ret == -1) {
#ifdef WIN32
            errorValue = WSAGetLastError();
#else
            errorValue = -1;
#endif
            qDebug() << "select(): Socket error.";
            return 0; // false
    }
    else if (!FD_ISSET(socketDesc, &fds)) {
        errorValue = 0;
        qDebug() << "NNetworkSocket: no data in buffer.";
        return 0; // false
    }
      
    //qDebug() << "NNetworkSocket: starting recv loop...";
    while (bytesLeft > 0) {
        bytesRead = recv(socketDesc, buffer, bytesLeft, 0);
        //qDebug() << "Bytes received:" << bytesRead;
        if (bytesRead == -1) {
#ifdef WIN32
            errorValue = WSAGetLastError();
#else
            errorValue = -1;
#endif
            qDebug() << "Socket error.";
            return 0; // false
        }
        else if (bytesRead == 0) {
            // the socket was closed on the client's side. Handle it.
            emit disconnected();
            open = false;
            return 2; // disconnect
        }
        
        // next run
        bytesLeft -= bytesRead;
        buffer += bytesRead;
    }
    
    // put bytes into the byte array now.
    //qDebug() << "Assign data to output buffer.";
    buf.clear();
    buffer -= count;
    buf.append(buffer, count);
    return 1; // true
}


// --- READ ---
// Reads all available bytes from the socket and returns it in the provided buffer.
// Returns true if successful, else false with the error available via error().
bool NNetworkSocket::read(QByteArray &buf) {
    if (!open) { return false; }
    char* buffer = new char[4096]; // 4 kB buffer
    int bytesRead = 0;
    buf.clear();
    
    // checking for data availability in the buffer
    fd_set fds;
    int ret = 0;
    int waitTimeUs = 10000000; // 10 seconds
    struct timeval timeout;
    struct timeval* timeoutPtr = 0;
    
    if (waitTimeUs > 0) {
        timeout.tv_sec = waitTimeUs / 1000000;
        timeout.tv_usec = waitTimeUs % 1000000;
        timeoutPtr = &timeout;
    }
    
    FD_ZERO(&fds);
    FD_SET(socketDesc, &fds);
    
    ret = select(socketDesc + 1, &fds, 0, 0, timeoutPtr);
    if (ret == -1) {
#ifdef WIN32
            errorValue = WSAGetLastError();
#else
            errorValue = -1;
#endif
            qDebug() << "select(): Socket error.";
            return false;
    }
    else if (!FD_ISSET(socketDesc, &fds)) {
        errorValue = 0;
        qDebug() << "NNetworkSocket: no data in buffer.";
        return false;
    }
      
    //qDebug() << "NNetworkSocket: starting recv loop...";
    while (1) {
        bytesRead = recv(socketDesc, buffer, 4096, 0);
        qDebug() << "Bytes received:" << bytesRead;
        if (bytesRead == -1) {
#ifdef WIN32
            errorValue = WSAGetLastError();
#else
            errorValue = -1;
#endif
            qDebug() << "Socket error.";
            return false;
        }
        else if (bytesRead == 0) {
            // the socket was closed on the client's side. Handle it.
            qDebug() << "Disconnected";
            emit disconnected();
            open = false;
            return true;
        }
        else {
            // put bytes into the byte array now.
            //qDebug() << "Assign data to output buffer.";
            buf.append(buffer, bytesRead);
            //qDebug() << "buf:" << buf;
        }
            
    }
    
    return true;
}


// --- WRITE ---
// Writes the provided buffer to the socket. Returns true if successful, else false.
// Errors can be read via error().
bool NNetworkSocket::write(QByteArray &buf) {
    qDebug() << "NNetworkSocket: starting write...";
    if (!open) { return false; }
    qDebug() << "NNetworkSocket: sending...";
    int err = send(socketDesc, buf.constData(), buf.length(), 0);
    if (err == -1) {
#ifdef WIN32
            errorValue = WSAGetLastError();
#else
            errorValue = -1;
#endif
            qDebug() << "Socket error.";
        emit error(QAbstractSocket::UnknownSocketError); // TODO: translate winsock/posix socket error code to Qt code.
        return false;
    }
    else if (err == 0) {
        // the socket was closed on the client's side. Handle it.
        qDebug() << "Socket was closed client-side";
        emit disconnected();
        open = false;
        return false;
    }
    
    qDebug() << "NNetworkSocket: Finished write.";
    return true;
}


// --- ERROR STRING ---
// Returns the last error as a string.
QString NNetworkSocket::errorString() {
#ifdef WIN32
    switch (errorValue) {
    case WSANOTINITIALISED:    
        return "Winsock wasn't initialized.";
        break;
    case WSAENETDOWN:
        return "No available network.";
        break;    
    case WSAEFAULT:
        return "The buf parameter is not completely contained in a valid part of the user address space.";
        break;
    case WSAENOTCONN:
        return "The socket is not connected.";
        break;
    case WSAEINTR:
        return "The (blocking) call was canceled through WSACancelBlockingCall.";
        break;
    case WSAEINPROGRESS: 
        return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
        break;    
    case WSAENETRESET:
        return "For a connection-oriented socket, this error indicates that the connection has been broken due to keep-alive activity that detected a failure while the operation was in progress. For a datagram socket, this error indicates that the time to live has expired.";
        break;    
    case WSAENOTSOCK:
        return "The descriptor is not a socket.";
        break;    
    case WSAEOPNOTSUPP:    
        return "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.";
        break;
    case WSAESHUTDOWN:
        return "The socket has been shut down; it is not possible to receive on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
        break;
    case WSAEWOULDBLOCK:
        return "The socket is marked as nonblocking and the receive operation would block.";
        break;
    case WSAEMSGSIZE:
        return "The message was too large to fit into the specified buffer and was truncated.";
        break;
    case WSAEINVAL:
        return "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.";
        break;
    case WSAECONNABORTED:
        return "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
        break;
    case WSAETIMEDOUT:
        return "The connection has been dropped because of a network failure or because the peer system failed to respond.";
        break;
    case WSAECONNRESET:
        return "The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket as it is no longer usable. On a UDP-datagram socket, this error would indicate that a previous send operation resulted in an ICMP \"Port Unreachable\" message.";
        break;
    default:
        return "Unknown error: " + QString::number(errorValue);
        break;
    }
#else
    // POSIX stuff here
    return "A socket error occurred.";
#endif    
}


// --- IS OPEN ---
// Allows one to enquire the connection status of this socket.
bool NNetworkSocket::isOpen() const {
    return open;
}


// --- ERROR ---
// Returns the last occurred error as an enum entry.
QAbstractSocket::SocketError NNetworkSocket::error() {
    return QAbstractSocket::UnknownSocketError; // TODO: translate native error to QAbstractSocket one.
}


// --- CLOSE ---
// Closes the socket connection, disconnects it and frees any resources.
void NNetworkSocket::close() {
#ifdef WIN32
    closesocket(socketDesc);
#else
    ::close(socketDesc);
#endif
    open = false;
    emit disconnected();
}
