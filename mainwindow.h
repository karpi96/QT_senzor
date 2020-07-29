#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <string>
#include <QDebug>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include "qcustomplot.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int readData();

public slots:
    void sendData();
    //int readData();

private slots:
    void readSerial();
    void updateVoltage(QString);
    void realtimeDataSlot();

private:
    Ui::MainWindow *ui;
    QSerialPort *arduino;
    static const quint16 arduino_uno_vendor_id = 9025;
    static const quint16 arduino_uno_product_id = 67;
    QByteArray serialData;
    QString serialBuffer;
    QString parsed_data;
    int voltageValue;
    void setupArduinoCom();

    void postFinished(QNetworkReply*);

     QTimer *dataTimer;
     QTimer *uploadTimer;


};

#endif // MAINWINDOW_H
