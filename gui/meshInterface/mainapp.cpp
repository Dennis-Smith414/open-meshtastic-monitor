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
    qDebug() << "MainApp constructor started";
    ui->setupUi(this);
    qDebug() << "ui->setupUi completed";

    // Check centralwidget
    if (!ui->centralwidget) {
        qDebug() << "ERROR: centralwidget is null!";
        return;
    }
    qDebug() << "centralwidget exists";

    // Log UI widget names
    qDebug() << "Available UI widgets:" << ui->centralwidget->findChildren<QWidget*>().count();
    for (QWidget* widget : ui->centralwidget->findChildren<QWidget*>()) {
        qDebug() << "Widget:" << widget->objectName();
    }

    // Verify layout
    if (ui->centralwidget->layout()) {
        qDebug() << "Centralwidget layout:" << ui->centralwidget->layout()->metaObject()->className();
        // Log layout contents
        QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->centralwidget->layout());
        if (gridLayout) {
            qDebug() << "Grid layout found, item count:" << gridLayout->count();
            for (int i = 0; i < gridLayout->count(); ++i) {
                QLayoutItem* item = gridLayout->itemAt(i);
                if (item && item->widget()) {
                    qDebug() << "Layout item" << i << "widget:" << item->widget()->objectName();
                }
            }
        }
    } else {
        qDebug() << "No layout found on centralwidget, creating new QVBoxLayout";
        QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);
        if (ui->label) {
            qDebug() << "Adding label to layout";
            mainLayout->addWidget(ui->label);
        } else {
            qDebug() << "ERROR: label is null!";
        }
        if (ui->packet_view) {
            qDebug() << "Adding packet_view to layout";
            mainLayout->addWidget(ui->packet_view);
            ui->packet_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        } else {
            qDebug() << "ERROR: packet_view is null!";
        }
        if (ui->pushButton) {
            qDebug() << "Adding pushButton to layout";
            mainLayout->addWidget(ui->pushButton);
            ui->pushButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        } else {
            qDebug() << "ERROR: pushButton is null!";
        }
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(10);
        ui->centralwidget->setLayout(mainLayout);
        qDebug() << "Layout created and set on centralwidget";
    }

    // Configure packet_view
    if (ui->packet_view) {
        qDebug() << "Configuring packet_view";
        ui->packet_view->setMaximumBlockCount(100);
        ui->packet_view->setReadOnly(true);
        ui->packet_view->appendPlainText("--- Application Started ---");
        qDebug() << "packet_view configured and initialized";
    } else {
        qDebug() << "ERROR: packet_view is null during configuration!";
    }

    // Resize to screen size
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        qDebug() << "Resizing window to screen size";
        QRect screenGeometry = screen->availableGeometry();
        this->resize(screenGeometry.width(), screenGeometry.height());
        qDebug() << "Window resized to" << screenGeometry.width() << "x" << screenGeometry.height();
    } else {
        qDebug() << "ERROR: No primary screen found!";
    }

    // Initialize meshHandler
    if (!meshHandler) {
        qDebug() << "WARNING: meshHandler is null, creating new instance";
        meshHandler = new meshtastic_handler(this);
        qDebug() << "meshHandler created";
    }
    qDebug() << "meshHandler initialized";

    // Signal connections (commented out for testing)

    qDebug() << "Connecting stateChanged signal";
    connect(meshHandler, &meshtastic_handler::stateChanged, this, &MainApp::onConnectionStateChanged);
    qDebug() << "Connected stateChanged signal";

    qDebug() << "Connecting packetReceived signal";
    connect(meshHandler, &meshtastic_handler::packetReceived, this, &MainApp::onPacketReceived);
    qDebug() << "Connected packetReceived signal";

    qDebug() << "Connecting logMessage signal";
    connect(meshHandler, &meshtastic_handler::logMessage, this, [this](const QString& msg, const QString& level) {
        qDebug() << "Log message received: [" << level << "]" << msg;
        if (ui->packet_view) {
            ui->packet_view->appendPlainText(QString("[%1] %2").arg(level).arg(msg));
            qDebug() << "Appended log message to packet_view";
        } else {
            qDebug() << "ERROR: packet_view is null in logMessage lambda!";
        }
    });
    qDebug() << "Connected logMessage signal";


    // Timer setup
    qDebug() << "Setting up timer";
    QTimer *timer = new QTimer(this);
    qDebug() << "Connecting timer timeout signal";
    connect(timer, &QTimer::timeout, this, &MainApp::updateConnectionStatusDisplay);
    qDebug() << "Connected timer timeout signal";
    timer->start(1000);
    qDebug() << "Timer started with 1000ms interval";

    qDebug() << "MainApp constructor completed";
}


// MainApp::MainApp(QWidget *parent)
//     : QMainWindow{parent}
//     , ui(new Ui::MainWindow)
// {
//     ui->setupUi(this);

//     if (!ui->centralwidget) {
//         qDebug() << "ERROR: centralwidget is null!";
//         return;
//     }

//     // if (!ui->centralwidget->layout()) {
//     //     qDebug() << "Creating layout in code since Designer layout failed";

//     //     QVBoxLayout *layout = new QVBoxLayout(ui->centralwidget);

//     //     // Add widgets in the order they appear in your UI
//     //     layout->addWidget(ui->label);
//     //     layout->addWidget(ui->packet_view);
//     //     layout->addWidget(ui->pushButton);

//     //     // Set layout properties to match your UI file
//     //     layout->setContentsMargins(10, 10, 10, 10);
//     //     layout->setSpacing(6);  // Default spacing

//     //     // Set size policies for proper scaling
//     //     ui->label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//     //     ui->packet_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//     //     ui->pushButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

//     //     qDebug() << "Layout created successfully";
//     // }

//     // if (ui->centralwidget && !ui->centralwidget->layout()) {
//     //     QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);

//     //     if (ui->packet_view) {
//     //         ui->packet_view->setParent(nullptr);
//     //         mainLayout->addWidget(ui->packet_view);
//     //     }

//     //     if (ui->pushButton) {
//     //         ui->pushButton->setParent(nullptr);
//     //         mainLayout->addWidget(ui->pushButton);
//     //     }

//     //     // Set layout margins and spacing
//     //     mainLayout->setContentsMargins(10, 10, 10, 10);
//     //     mainLayout->setSpacing(10);
//     // }

//     // if (!ui->centralwidget->layout()) {
//     //     QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);
//     //     if (ui->packet_view) {
//     //         mainLayout->addWidget(ui->packet_view);
//     //         ui->packet_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//     //     } else {
//     //         qDebug() << "ERROR: packet_view is null!";
//     //     }
//     //     if (ui->pushButton) {
//     //         mainLayout->addWidget(ui->pushButton);
//     //         ui->pushButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//     //     } else {
//     //         qDebug() << "ERROR: pushButton is null!";
//     //     }
//     //     mainLayout->setContentsMargins(10, 10, 10, 10);
//     //     mainLayout->setSpacing(10);
//     //     ui->centralwidget->setLayout(mainLayout);
//     // }

//     // if (ui->packet_view) {
//     //     ui->packet_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//     // }
//     // if (ui->pushButton) {
//     //     ui->pushButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//     // }

//     // if (ui->centralwidget && !ui->centralwidget->layout()) {
//     //     qDebug() << "No layout found on central widget - this is likely your problem!";
//     //     // You'll need to set up a layout here or in Designer
//     // }

//     // //textbox viewer
//     // ui->packet_view->setMaximumBlockCount(100);
//     // ui->packet_view->setReadOnly(true);

//     qDebug() << "MainApp constructor started";
//     ui->setupUi(this);
//     qDebug() << "ui->setupUi completed";

//     // Check centralwidget
//     if (!ui->centralwidget) {
//         qDebug() << "ERROR: centralwidget is null!";
//         return;
//     }
//     qDebug() << "centralwidget exists";

//     // Create layout if none exists
//     if (!ui->centralwidget->layout()) {
//         qDebug() << "No layout found on centralwidget, creating new QVBoxLayout";
//         QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);
//         if (ui->packet_view) {
//             qDebug() << "Adding packet_view to layout";
//             mainLayout->addWidget(ui->packet_view);
//             ui->packet_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//         } else {
//             qDebug() << "ERROR: packet_view is null!";
//         }
//         if (ui->pushButton) {
//             qDebug() << "Adding pushButton to layout";
//             mainLayout->addWidget(ui->pushButton);
//             ui->pushButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//         } else {
//             qDebug() << "ERROR: pushButton is null!";
//         }
//         mainLayout->setContentsMargins(10, 10, 10, 10);
//         mainLayout->setSpacing(10);
//         ui->centralwidget->setLayout(mainLayout);
//         qDebug() << "Layout created and set on centralwidget";
//     } else {
//         qDebug() << "Centralwidget already has layout:" << ui->centralwidget->layout()->metaObject()->className();
//     }

//     // Configure packet_view
//     if (ui->packet_view) {
//         qDebug() << "Configuring packet_view";
//         ui->packet_view->setMaximumBlockCount(100);
//         ui->packet_view->setReadOnly(true);
//         ui->packet_view->appendPlainText("--- Application Started ---");
//         qDebug() << "packet_view configured and initialized";
//     }

//     // Resize to screen size
//     QScreen *screen = QGuiApplication::primaryScreen();
//     if (screen) {
//         qDebug() << "Resizing window to screen size";
//         QRect screenGeometry = screen->availableGeometry();
//         this->resize(screenGeometry.width(), screenGeometry.height());
//         qDebug() << "Window resized to" << screenGeometry.width() << "x" << screenGeometry.height();
//     } else {
//         qDebug() << "ERROR: No primary screen found!";
//     }

//     // Initialize meshHandler
//     if (!meshHandler) {
//         qDebug() << "WARNING: meshHandler is null, creating new instance";
//         meshHandler = new meshtastic_handler(this); // Adjust if different initialization is needed
//     }
//     qDebug() << "meshHandler initialized";

//     // Signal connections with debug
//     // qDebug() << "Connecting stateChanged signal";
//     // connect(meshHandler, &meshtastic_handler::stateChanged, this, &MainApp::onConnectionStateChanged);
//     // // qDebug() << "Connected stateChanged signal";

//     // qDebug() << "Connecting packetReceived signal";
//     // connect(meshHandler, &meshtastic_handler::packetReceived, this, &MainApp::onPacketReceived);
//     // qDebug() << "Connected packetReceived signal";

//     // qDebug() << "Connecting logMessage signal";
//     // connect(meshHandler, &meshtastic_handler::logMessage, this, [this](const QString& msg, const QString& level) {
//     //     qDebug() << "Log message received: [" << level << "]" << msg;
//     //     if (ui->packet_view) {
//     //         ui->packet_view->appendPlainText(QString("[%1] %2").arg(level).arg(msg));
//     //         qDebug() << "Appended log message to packet_view";
//     //     } else {
//     //         qDebug() << "ERROR: packet_view is null in logMessage lambda!";
//     //     }
//     // });
//     // qDebug() << "Connected logMessage signal";

//   //   //signal init
//   //   connect(meshHandler, &meshtastic_handler::stateChanged, this, &MainApp::onConnectionStateChanged);
//   //   connect(meshHandler, &meshtastic_handler::packetReceived, this, &MainApp::onPacketReceived);
//      // connect(meshHandler, &meshtastic_handler::logMessage, this, [](const QString& msg, const QString& level) {
//      //     qDebug() << "[" << level << "]" << msg;
//      // });

//   //   connect(meshHandler, &meshtastic_handler::packetReceived, this, [](const QJsonObject& packet) {
//   //       qDebug() << "DEBUG: packetReceived signal was emitted!";
//   //   });

//   //   //connect(meshHandler, &meshtastic_handler::rawDataReceived, this, &MainApp::onRawDataReceived);

//   //   connect(meshHandler, &meshtastic_handler::logMessage, this, [this](const QString& msg, const QString& level) {
//   //       qDebug() << "[" << level << "]" << msg;
//   //       ui->packet_view->appendPlainText(QString("[%1] %2").arg(level).arg(msg));
//   //   });

//   //   //updating timer
//   //   QTimer *timer = new QTimer(this);
//   //   connect(timer, &QTimer::timeout, this, &MainApp::updateConnectionStatusDisplay);
//   //   connect(meshHandler, &meshtastic_handler::packetReceived,
//   //           this, &MainApp::onPacketReceived);

//   // //  qDebug() << "Timer connection successful?" << connect;onPacketReceived
//   //   timer->start(1000);

//     ui->packet_view->appendPlainText("--- Application Started ---");
// }

MainApp::~MainApp()
{
    delete ui;
}

void MainApp::updateConnectionStatusDisplay()
{
    qDebug() << "updateConnectionStatusDisplay() called by timer!"; // Add this line

  //  meshtastic_handler::Connection_Status currentStatus = meshHandler->state();

    //meshtastic_handler::packetReceived(packet
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
    QString usb_path = "/dev/ttyUSB0";   if (meshHandler->isRunning()) {
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



void MainApp::onRawDataReceived(const QString& rawData) {
    QString cleanData = rawData;

    QRegularExpression ansiRegex("\x1B\\[[0-9;]*m");
    cleanData.remove(ansiRegex);

    // Display in the text box
    ui->packet_view->appendPlainText(cleanData);

    // Auto-scroll to bottom
    QTextCursor cursor = ui->packet_view->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->packet_view->setTextCursor(cursor);
}

// void MainApp::onPacketReceived(const QJsonObject& packet) {
//     qDebug() << "JSON Packet Received!" << QJsonDocument(packet).toJson(QJsonDocument::Compact);

//     if (packet.isEmpty()) {
//         qDebug() << "Empty JSON packet received";
//         return;
//     }

//     static int packetCount = 0;
//     packetCount++;

//     QString fromID = packet["fromID"].toString("Unknown");
//     QString toID = packet["toID"].toString("Unknown");
//     QJsonObject decoded = packet["decoded"].toObject();
//     QString portnum = decoded["portnum"].isDouble() ? QString::number(decoded["portnum"].toInt()) : "Unknown";
//     QString messageText = decoded["text"].toString("");
//     int rssi = packet["rxRssi"].isDouble() ? packet["rxRssi"].toInt() : 0;
//     double snr = packet["rxSnr"].isDouble() ? packet["rxSnr"].toDouble() : 0.0;

//     QString displayText;
//     displayText += QString("=== JSON Packet #%1 ===\n").arg(packetCount);
//     displayText += QString("From: %1\n").arg(fromID);
//     displayText += QString("To: %1\n").arg(toID);
//     if (rssi != 0) {
//         displayText += QString("RSSI: %1 dBm\n").arg(rssi);
//     }
//     if (snr != 0.0) {
//         displayText += QString("SNR: %1 dB\n").arg(snr);
//     }
//     displayText += QString("Port: %1\n").arg(portnum);
//     if (!messageText.isEmpty()) {
//         displayText += QString("Message: %1\n").arg(messageText);
//     }
//     if (decoded["batteryLevel"].isDouble()) {
//         displayText += QString("Battery: %1%\n").arg(decoded["batteryLevel"].toInt());
//     }
//     if (decoded["latitude"].isDouble() && decoded["longitude"].isDouble()) {
//         displayText += QString("Position: %1, %2\n")
//         .arg(decoded["latitude"].toDouble(), 0, 'f', 6)
//             .arg(decoded["longitude"].toDouble(), 0, 'f', 6);
//     }

//     ui->packet_view->appendPlainText(displayText);
// }


void MainApp::onPacketReceived(const QJsonObject& packet) {



}


// void MainApp::onPacketReceived(const QJsonObject& packet) {
//     static int packetCount = 0;
//     packetCount++;

//     QString Log;

//     if (packet.contains("fromId")) {
//         Log += QString("From: %1 ").arg(packet["fromId"].toString());
//     }

//     if (packet.contains("toId")) {
//         Log += QString("To: %1 ").arg(packet["toId"].toString());
//     }

//     if (packet.contains("rxRssi")) {
//         Log += QString("RSSI: %1dBm ").arg(packet["rxRssi"].toInt());
//     }

//     if (packet.contains("rxSnr")) {
//         Log += QString("SNR: %1dB ").arg(packet["rxSnr"].toDouble());
//     }

//     if (packet.contains("hopLimit")) {
//         Log += QString("Hops: %1 ").arg(packet["hopLimit"].toInt());
//     }

//     // Create formatted string for the text box
//     QString displayText;
//     displayText += QString("=== JSON Packet #%1 ===\n").arg(packetCount);

//     displayText += QString("Log: %1\n").arg(Log);

//     displayText += QString("Raw JSON: %1\n").arg(QString::fromUtf8(QJsonDocument(packet).toJson(QJsonDocument::Compact)));
//     displayText += "------------------------\n";
//     ui->packet_view->appendPlainText(displayText);
// }


// void MainApp::onPacketReceived(const QJsonObject& packet) {
//     qDebug() << "JSON Packet Received!";

//     // Only process if we actually have a valid JSON packet
//     if (packet.isEmpty()) {
//         qDebug() << "Empty JSON packet received";
//         return;
//     }

//     // Extract packet information
//     QString fromID = packet["fromID"].toString();
//     int rssi = packet["rxRssi"].toInt();
//     QJsonObject decoded = packet["decoded"].toObject();
//     QString messageText = decoded["text"].toString();
//     QString portnum = decoded["portnum"].toString();

//     // Update packet counter
//     static int packetCount = 0;
//     packetCount++;


//     // Create formatted string for the text box
//     QString displayText;
//     displayText += QString("=== JSON Packet #%1 ===\n").arg(packetCount);

//     displayText += QString("Raw JSON: %1\n").arg(QString::fromUtf8(QJsonDocument(packet).toJson(QJsonDocument::Compact)));
//     displayText += "------------------------\n";
//     ui->packet_view->appendPlainText(displayText);


    // displayText += QString("From: %1\n").arg(fromID);
    // displayText += QString("To: %1\n").arg(packet["toId"].toString());
    // displayText += QString("RSSI: %1 dBm\n").arg(rssi);

    // if (packet["rxRssi"] != QJsonValue::Null) {
    //     displayText += QString("RSSI: %1 dBm\n").arg(rssi);
    // }

    // if (packet["rxSnr"] != QJsonValue::Null) {
    //     displayText += QString("SNR: %1 dB\n").arg(packet["rxSnr"].toDouble());
    // }

    // displayText += QString("Port: %1\n").arg(portnum);

    // // if (!messageText.isEmpty()) {
    // //     displayText += QString("Message: %1\n").arg(messageText);
    // // }

    // displayText += QString("Message: %1\n").arg(messageText);

    // if (decoded["batteryLevel"] != QJsonValue::Null) {
    //     displayText += QString("Battery: %1%\n").arg(decoded["batteryLevel"].toInt());
    // }

    // if (decoded["latitude"] != QJsonValue::Null && decoded["longitude"] != QJsonValue::Null) {
    //     displayText += QString("Position: %1, %2\n")
    //     .arg(decoded["latitude"].toDouble(), 0, 'f', 6)
    //         .arg(decoded["longitude"].toDouble(), 0, 'f', 6);
    // }
//}

