/*
    socketworker.h - worker header file of the NNetworkSocket sample application.
    
    Revision 0
    
    Notes:
            - 
            
    2013/08/15, Maya Posch
    (c) Nyanko.ws
*/


#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H

#include <QObject>

#include "nnetworksocket.h"

class SocketWorker : public QObject {
    Q_OBJECT
public:
    explicit SocketWorker(QObject *parent = 0);
    
signals:
    void pageContent(QString html);
    void error(QString err);
    void finished();
    
public slots:
    void start();
    
private:
    NNetworkSocket socket;
};

#endif // SOCKETWORKER_H
