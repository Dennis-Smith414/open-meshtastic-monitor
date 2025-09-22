#include "mainapp.h"
#include "ui_MainApp.h"
#include <QPainter>
#include "meshtastic_handler.h"
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QPropertyAnimation>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

MainApp::MainApp(QWidget *parent)
    : QMainWindow{parent}
    , ui(new Ui::MainWindow)
    , meshHandler(nullptr)
{
    //main constuctor
    ui->setupUi(this);

    //set debug mode to off on start up (Do this in the constuctor as well)
    ui->debug_check->setChecked(false);
    //Dynamically resize window
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        qDebug() << "Resizing window to screen size";
        QRect screenGeometry = screen->availableGeometry();
        this->resize(screenGeometry.width(), screenGeometry.height());
        qDebug() << "Window resized to" << screenGeometry.width() << "x" << screenGeometry.height();
    } else {
        qDebug() << "ERROR: No primary screen found!";
    }

    // Init meshHandler
    if (!meshHandler) {
        qDebug() << "WARNING: meshHandler is null, creating new instance";
        meshHandler = new meshtastic_handler(this);
        qDebug() << "meshHandler created";
    }

    // Signal connections
    //State signal
    connect(meshHandler, &meshtastic_handler::stateChanged, this, &MainApp::onConnectionStateChanged);

    //Log message signal
    connect(meshHandler, &meshtastic_handler::logMessage, this, [this](const QString& msg, const QString& level) {
        qDebug() << "Log message received: [" << level << "]" << msg;
        if (ui->packet_view) {
            ui->packet_view->appendPlainText(QString("[%1] %2").arg(level).arg(msg));
       //     qDebug() << "Appended log message to packet_view";
        } else {
            qDebug() << "ERROR: packet_view is null in logMessage lambda!";
        }
    });

    connect(meshHandler, &meshtastic_handler::logBattery, this, [this](const QString& msg) {
        QString new_read = "Battery: " + msg + "%";
        qDebug() << "Updating battery status: " << new_read;
        qDebug() << "adding the battery thing";
        if (ui->battery_status_label) {
             ui->battery_status_label->setText(new_read);
        } else {
            qDebug() << "ERROR: noting in battery field";
        }
    });

}

MainApp::~MainApp()
{
    delete ui;
}

//Paint event for background image
void MainApp::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    QPainter painter(this);
    QPixmap background(":/images/topo-map1.jpeg");

    QPixmap scaledBackground = background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    painter.drawPixmap(0, 0, scaledBackground);
}

//Init the connection should be moved to a setting screen on or off could be a saved stae
void MainApp::on_pushButton_clicked()
{
    QString usb_path;
    if (meshHandler->isRunning()) {
        qDebug() << "Stopping the connection..";
        meshHandler->stopMeshtastic();
        ui->pushButton->setText("Start MQTT");
    } else {
        qDebug() << "Starting the connection";
        meshHandler->startMeshtastic(usb_path);
        ui->pushButton->setText("Connecting...");
        ui->pushButton->setEnabled(false);
    }
}

//Turn full packet debug on or off


//State machine for setting up a connection
void MainApp::onConnectionStateChanged(meshtastic_handler::Connection_Status status) {
    switch (status) {
    case meshtastic_handler::Disconnected:
        ui->pushButton->setText("Start Packet View");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Connecting:
        ui->pushButton->setText("Stop Packet View");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Connected:
        ui->pushButton->setText("Stop Packet View");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Error:
        ui->pushButton->setText("Error: Retry Connection");
        ui->pushButton->setEnabled(true);
        break;
    }
}

void MainApp::on_debug_check_clicked(bool checked)
{
   meshHandler->set_debug_status(checked);
}

