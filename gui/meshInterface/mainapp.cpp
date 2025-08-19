#include "mainapp.h"
#include "ui_MainApp.h"
#include <QPainter>
#include "meshtastic_handler.h"
#include <QProcess>
#include <QString>

MainApp::MainApp(QWidget *parent)
    : QMainWindow{parent}
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Mesh Monitor - Main App");
    meshHandler = new meshtastic_handler(this);

    //signal init
    connect(meshHandler, &meshtastic_handler::stateChanged, this, &MainApp::onConnectionStateChanged);
    connect(meshHandler, &meshtastic_handler::packetReceived, this, &MainApp::onPacketReceived);
    connect(meshHandler, &meshtastic_handler::logMessage, this, [](const QString& msg, const QString& level) {
        qDebug() << "[" << level << "]" << msg;
    });
    connect(meshHandler, &meshtastic_handler::packetReceived, this, [](const QJsonObject& packet) {
        qDebug() << "DEBUG: packetReceived signal was emitted!";
    });
}

MainApp::~MainApp()
{
    delete ui;
}

void MainApp::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    QPainter painter(this);
    QPixmap background(":/images/topo-map1.jpeg");

    QPixmap scaledBackground = background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Paint it
    painter.drawPixmap(0, 0, scaledBackground);
}


void MainApp::on_pushButton_clicked()
{
    //QString script_path = "/home/KINGSNEEDTHETERRIBLE/repos/open-meshtastic-monitor/mqtt/start_mqtt.sh";
    QString usb_path = "/dev/ttyUSB0";
    if (meshHandler->isRunning()) {
        qDebug() << "Stopping the connection..";
        meshHandler->stopMeshtastic();
        ui->pushButton->setText("Start MQTT");
    } else {
        qDebug() << "Starting the connection";
        meshHandler->startMeshtastic(usb_path);
        ui->pushButton->setText("Connecting...");
        ui->pushButton->setEnabled(false);
        pipe_test();
    }

}

void MainApp::pipe_test() {
    qDebug() << "=== TESTING PIPE SYSTEM ===";

    //QProcess::startDetached("/bin/bash", QStringList() << "/home/KINGSNEEDTHETERRIBLE/repos/open-meshtastic-monitor/mqtt/start_mqtt.sh");




}

void MainApp::onConnectionStateChanged(meshtastic_handler::Connection_Status status) {
    switch (status) {
    case meshtastic_handler::Disconnected:
        qDebug() << "DISCONNECTED";
        ui->pushButton->setText("Start MQTT");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Connecting:
        qDebug() << "Connecting";
        ui->pushButton->setText("Stop Meshtastic");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Connected:
        qDebug() << "Connected! Waiting for packets...";
        ui->pushButton->setText("Stop Meshtastic");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Error:
        qDebug() << "Connection Error!";
        ui->pushButton->setText("Error: Retry Connection");
        ui->pushButton->setEnabled(true);
        break;
    }
}

void MainApp::onPacketReceived(const QJsonObject& packet) {
    qDebug() << "Packet Recived!";
    qDebug() << "From:" << packet["fromID"].toString();
    qDebug() << "RSSI:" << packet["rxRssi"].toInt() << "dBm";

    QJsonObject decoded = packet["decoded"].toObject();
    if(!decoded["text"].toString().isEmpty()) {
        qDebug() << "Message:" << decoded["text"].toString();
    }

    //verify we are getting data
    static int packetCount = 0;
    packetCount++;
    qDebug() << "Total number packets" << packetCount;
}

void MainApp::testPacket() {
    QJsonObject testPacket;
    testPacket["fromID"] = "test123";
    testPacket["rxRssi"] = -50;
    QJsonObject decoded;
    decoded["text"] = "Test message";
    testPacket["decoded"] = decoded;

    onPacketReceived(testPacket); // Call directly to test
}


