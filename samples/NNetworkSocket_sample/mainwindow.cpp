/*
    mainwindow.cpp - main file of the NNetworkSocket sample application.
    
    Revision 0
    
    Features:
            - Demonstrates the NNetworkSocket usage.
            
    Notes:
            - 
            
    2013/08/15, Maya Posch
    (c) Nyanko.ws
*/


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "socketworker.h"


#include <QCloseEvent>
#include <QThread>
#include <QMessageBox>


// --- CONSTRUCTOR ---
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    
    // menu connections
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
    
    // other connections
    connect(ui->pushButton, SIGNAL(pressed()), this, SLOT(fetchPage()));
}


// --- DECONSTRUCTOR ---
MainWindow::~MainWindow() {
    delete ui;
}


// --- ERROR STRING ---
// Receives and displays an error string.
void MainWindow::errorString(QString err) {
    QMessageBox::critical(this, tr("Error"), err);
}


// --- QUIT ---
// Exits the application.
void MainWindow::quit() {
    exit(0);
}


// --- FETCH PAGE ---
// Creates a new thread in which an NNetworkSocket fetches the HTML of the page
// we'll display.
void MainWindow::fetchPage() {
    QThread* thread = new QThread;
    SocketWorker* worker = new SocketWorker;
    worker->moveToThread(thread);
    connect(worker, SIGNAL(pageContent(QString)), this, SLOT(displayPage(QString)));
    connect(worker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    connect(thread, SIGNAL(started()), worker, SLOT(start()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}


// --- DISPLAY PAGE ---
// Displays the received HTML content.
void MainWindow::displayPage(QString html) {
    ui->textEdit->setHtml(html);
}


// --- CLOSE EVENT ---
// Triggered when the window is closed.
void MainWindow::closeEvent(QCloseEvent *event) {
    quit();
    event->ignore();
}
