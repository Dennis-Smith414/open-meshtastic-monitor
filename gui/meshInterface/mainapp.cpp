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
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>

MainApp::MainApp(QWidget *parent)
    : QMainWindow{parent}
    , ui(new Ui::MainWindow)
    , meshHandler(nullptr)
{
    //main constuctor
    ui->setupUi(this);

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
    connect(ui->debug_check, &QCheckBox::toggled, this, &MainApp::on_debug_check_clicked);

    connect(meshHandler, &meshtastic_handler::stateChanged, this, &MainApp::onConnectionStateChanged);

    //Log message signal
    connect(meshHandler, &meshtastic_handler::logMessage, this, [this](const QString& msg, const QString& level) {
        qDebug() << "Log message received: [" << level << "]" << msg + "\r\n";
        if (ui->packet_view) {
            ui->packet_view->appendPlainText(QString("[%1] %2\r\n").arg(level).arg(msg));
       //     qDebug() << "Appended log message to packet_view";
        } else {
            qDebug() << "ERROR: packet_view is null in logMessage lambda!";
        }
    });

    //Battery life signal
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

    connect(meshHandler, &meshtastic_handler::positionUpdate,
            this, &MainApp::onPositionUpdate);

    connect(meshHandler, &meshtastic_handler::logNodesOnline, this, [this](const QString& num) {
        QString new_num = "Nodes Online: " + num;
        if (ui->nodes_online) {
            ui->nodes_online->setText(new_num);
        }
    });

    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "BUTTON CLICKED - Current text:" << ui->pushButton->text();
    });


    setupMap();
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

void MainApp::setupMap() {
    mapView = new QWebEngineView(ui->Map);

    // Wait for page to load before allowing map updates
    connect(mapView, &QWebEngineView::loadFinished, [this](bool ok) {
        qDebug() << "Map loaded successfully:" << ok;
        mapReady = ok;
    });

    QVBoxLayout* mapLayout = new QVBoxLayout(ui->Map);
    mapLayout->addWidget(mapView);
    mapLayout->setContentsMargins(0, 0, 0, 0);

    mapView->load(QUrl("qrc:/map.html"));
}

void MainApp::on_pushButton_clicked()
{
    QString usb_path;
    if (meshHandler->isRunning()) {
        qDebug() << "Stopping the connection..";
        meshHandler->stopMeshtastic();
        ui->pushButton->setText("Start Packet View");
    } else {
        qDebug() << "Starting the connection";
        meshHandler->startMeshtastic(usb_path);
    }
}

void MainApp::onMapLoadFinished(bool success) {
    if (success) {
        qDebug() << "Map loaded, injecting addNode function...";

        // Inject the addNode function directly
        QString addNodeFunction = R"(
            var nodeMarkers = {};

            function addNode(nodeId, lat, lon, isNewNode) {
                console.log('Adding node:', nodeId, 'at', lat, lon);

                if (nodeMarkers[nodeId]) {
                    map.removeLayer(nodeMarkers[nodeId]);
                }

                var marker = L.marker([lat, lon]).addTo(map);

                var popupContent = '<b>Node: ' + nodeId + '</b><br>' +
                                  'Latitude: ' + lat.toFixed(6) + '<br>' +
                                  'Longitude: ' + lon.toFixed(6) + '<br>' +
                                  'Last Update: ' + new Date().toLocaleString();

                marker.bindPopup(popupContent);
                nodeMarkers[nodeId] = marker;

                if (isNewNode || Object.keys(nodeMarkers).length === 1) {
                    map.setView([lat, lon], 12);
                }

                return true;
            }

            window.mapReady = true;
            console.log('addNode function injected and map ready');
        )";

        mapView->page()->runJavaScript(addNodeFunction, [this](const QVariant &result) {
            QTimer::singleShot(500, [this]() {
                checkMapReady();
            });
        });
    } else {
        qDebug() << "Failed to load map HTML";
    }
}

void MainApp::onPositionUpdate(const QString& nodeId, double lat, double lon) {
    qDebug() << "MainApp received position update for" << nodeId << "at" << lat << "," << lon;
    updateNodeOnMap(nodeId, lat, lon);
}

void MainApp::updateNodeOnMap(const QString& nodeId, double lat, double lon) {
    if (!mapReady) {
        qDebug() << "Map not ready yet, skipping update for node:" << nodeId;
        return;
    }

    QString jsCode = QString("addNode('%1', %2, %3, false);")
                         .arg(nodeId)
                         .arg(lat, 0, 'f', 6)
                         .arg(lon, 0, 'f', 6);

    qDebug() << "Executing JS:" << jsCode;
    mapView->page()->runJavaScript(jsCode, [nodeId](const QVariant &result) {
        if (result.toBool()) {
            qDebug() << "Successfully added/updated node:" << nodeId;
        } else {
            qDebug() << "Failed to add/update node:" << nodeId;
        }
    });
}

void MainApp::checkMapReady() {
    mapView->page()->runJavaScript("window.mapReady", [this](const QVariant &result) {
        if (result.toBool()) {
            mapReady = true;
            qDebug() << "Map is ready for updates";
        } else {
            qDebug() << "Map not yet ready, checking again...";
            QTimer::singleShot(1000, [this]() {
                checkMapReady();
            });
        }
    });
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
         qDebug() << "Setting button to 'Connecting'";
        ui->pushButton->setText("Connecting");
        ui->pushButton->setEnabled(true);
        break;

    case meshtastic_handler::Connected:
        ui->pushButton->setText("Stop Packet View");
        ui->pushButton->setEnabled(true);
        ui->battery_status_label->setText("Battery: loading...");
        ui->nodes_online->setText("Nodes Online: loading...");
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

void MainApp::on_clear_terminal_button_clicked()
{
    ui->packet_view->clear();
}

void MainApp::on_saveButton_clicked() {
    QString log = ui->packet_view->toPlainText();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << log;
        file.close();
        QMessageBox::information(this, tr("Success"), tr("File saved successfully!"));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file!."));
    }
}

