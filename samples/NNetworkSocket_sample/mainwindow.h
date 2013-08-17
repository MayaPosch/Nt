/*
    mainwindow.h - main header file of the NNetworkSocket sample application.
    
    Revision 0
    
    Notes:
            - 
            
    2013/08/15, Maya Posch
    (c) Nyanko.ws
*/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
public slots:
    void errorString(QString err);
    
private slots:
    void quit();
    void fetchPage();
    void displayPage(QString html);
    
protected:
    void closeEvent(QCloseEvent* event);
    
private:    
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
