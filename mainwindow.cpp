#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupArduinoCom();

    dataTimer = new QTimer(this);

    ui->customPlot->addGraph(); // blue line
    ui->customPlot->graph(0)->setPen(QPen(QColor(40, 110, 255)));


    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%s");
    ui->customPlot->xAxis->setTicker(timeTicker);
    ui->customPlot->axisRect()->setupFullAxesBox();
    ui->customPlot->yAxis->setRange(0, 1024);
    ui->customPlot->yAxis->setLabel("ADC value");

    uploadTimer = new QTimer(this);
    connect(uploadTimer, SIGNAL(timeout()), this, SLOT(sendData()));
    //connect(uploadTimer, SIGNAL(timeout()), this, SLOT(readData()));
    uploadTimer->start(100);


    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
    connect(dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    dataTimer->start(10); // Interval 10 means to refresh as fast as possible

}

MainWindow::~MainWindow()
{

    if(arduino->isOpen()){
           arduino->close(); //    Close the serial port if it's open.
       }

    delete ui;
}



void MainWindow::setupArduinoCom(){
    arduino = new QSerialPort(this);
    serialBuffer = "";
    parsed_data = "";
    voltageValue = 0;


    bool arduino_is_available = false;
    QString arduino_uno_port_name;



    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
        //  check if the serialport has both a product identifier and a vendor identifier
        if(serialPortInfo.hasProductIdentifier() && serialPortInfo.hasVendorIdentifier()){
            //  check if the product ID and the vendor ID match those of the arduino uno
            if((serialPortInfo.productIdentifier() == arduino_uno_product_id)
                    && (serialPortInfo.vendorIdentifier() == arduino_uno_vendor_id)){
                arduino_is_available = true; //    arduino uno is available on this port
                arduino_uno_port_name = serialPortInfo.portName();
            }
        }
    }

    /*
     *  Open and configure the arduino port if available
     */
    if(arduino_is_available){
        qDebug() << "Found the arduino port...\n";
        arduino->setPortName(arduino_uno_port_name);
        arduino->open(QSerialPort::ReadOnly);
        arduino->setBaudRate(QSerialPort::Baud9600);
        arduino->setDataBits(QSerialPort::Data8);
        arduino->setFlowControl(QSerialPort::NoFlowControl);
        arduino->setParity(QSerialPort::NoParity);
        arduino->setStopBits(QSerialPort::OneStop);
        QObject::connect(arduino, SIGNAL(readyRead()), this, SLOT(readSerial()));
    }else{
        qDebug() << "Couldn't find the correct port for the arduino.\n";
        QMessageBox::information(this, "Serial Port Error", "Couldn't open serial port to arduino.");
    }
}

void MainWindow::readSerial()
{
    /*
     * readyRead() doesn't guarantee that the entire message will be received all at once.
     * The message can arrive split into parts.  Need to buffer the serial data and then parse for the temperature value.
     *
     */
    QStringList buffer_split = serialBuffer.split(","); //  split the serialBuffer string, parsing with ',' as the separator

    //  Check to see if there less than 3 tokens in buffer_split.
    //  If there are at least 3 then this means there were 2 commas,
    //  means there is a parsed voltage value as the second token (between 2 commas)
    if(buffer_split.length() < 3){
        // no parsed value yet so continue accumulating bytes from serial in the buffer.
        serialData = arduino->readAll();
        serialBuffer = serialBuffer + QString::fromStdString(serialData.toStdString());
        serialData.clear();
    }else{
        // the second element of buffer_split is parsed correctly, update the voltage value on lcdNumber
        serialBuffer = "";
        //qDebug() << buffer_split << "\n";
        parsed_data = buffer_split[1];

        qDebug() << "Voltage: " << parsed_data << "\n";
        updateVoltage(parsed_data);
    }

}

void MainWindow::updateVoltage(QString sensor_reading)
{
    //  update the value displayed on the lcdNumber
    ui->lcdNumber->display(sensor_reading);
}


void MainWindow::realtimeDataSlot(){

    static QTime time(QTime::currentTime());
    double key = time.elapsed()/1000.0;
    static double lastPointKey = 0;
    voltageValue = parsed_data.toInt();
    //qDebug( "the key before if is: %f", key );

    if(key - lastPointKey > 0.002)
    {
        ui->customPlot->graph(0)->addData(key,voltageValue);
        lastPointKey = key;
    }

    /* make key axis range scroll right with the data at a constant range of 8. */
    ui->customPlot->graph(0)->rescaleValueAxis();
    ui->customPlot->xAxis->setRange(key, 8, Qt::AlignRight);
    ui->customPlot->replot();
   // qDebug( "the key after if is : %f", key );
}

void MainWindow::sendData(){


   /* int data1;
    data1 = voltageValue;
    QString str = QString::number(data1);
*/

    QUrlQuery postData;
    postData.addQueryItem("user", "proba2");
    postData.addQueryItem("value1", parsed_data);

    QNetworkRequest req(QUrl("http://sensortracker.000webhostapp.com/post_2.php"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkAccessManager *man = new QNetworkAccessManager(this);
    man->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());

    connect(man, &QNetworkAccessManager::finished, this, &MainWindow::postFinished);

    QString sent;
    sent = postData.toString();

   // qDebug() << "Update..";
   // qDebug() << "Sent Data :: " << sent;
}

void MainWindow::postFinished(QNetworkReply *reply){

    QString response;

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response = "Error code : " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() + " -- Error string : " + reply->errorString() + "\n";

    qDebug() << "Respons :: " << response << "\n";
    //qDebug() << "Sent Data :: " << postData
}


int MainWindow::readData(){
    return voltageValue;
}
