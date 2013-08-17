/*
    socketworker.cpp - worker implementation file of the NNetworkSocket sample application.
    
    Revision 0
    
    Notes:
            - 
            
    2013/08/15, Maya Posch
    (c) Nyanko.ws
*/


#include "socketworker.h"


// --- CONSTRUCTOR ---
SocketWorker::SocketWorker(QObject *parent) : QObject(parent) {
    //
}


// --- START ---
// Begins to fetch the content from the hard-coded address. Emits it once finished.
void SocketWorker::start() {
    QString address = "qt-project.org";
    int port = 80;
    
    // Create the socket instance.
    if (!socket.setProperties(address, port, SOCKET_FAMILY_UNSPEC, SOCKET_TYPE_STREAM)) {
        emit error("Failed to create the socket: " + socket.errorString());
        return;
    }
    
    // Connect.
    if (!socket.connect()) {
        emit error("Failed to connect on the socket: " + socket.errorString());
        return;
    }
    
    // Send the HTTP request.
    QByteArray req;
    req = "GET / HTTP/1.1\r\n";
    req += "Host: " + address.toLatin1() + "\r\n\r\n";
    if (!socket.write(req)) {
        emit error("Failed to write to the socket: " + socket.errorString());
        return;
    }
    
    // Receive.
    QByteArray buffer;
    if (!socket.read(buffer)) {
        emit error("Failed to read data: " + socket.errorString());
        return;
    }
    
    // Emit.
    QString out = buffer;
    emit pageContent(out);
    socket.close();
    emit finished();
}
