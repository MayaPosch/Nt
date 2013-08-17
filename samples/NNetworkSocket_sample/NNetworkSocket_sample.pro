#-------------------------------------------------
#
# Project created by QtCreator 2013-08-15T20:12:26
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NNetworkSocket_sample
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    nnetworksocket.cpp \
    socketworker.cpp

HEADERS  += mainwindow.h \
    nnetworksocket.h \
    socketworker.h

FORMS    += mainwindow.ui
