#ifndef MAINAPP_H
#define MAINAPP_H

#include "meshtastic_handler.h"
#include <QMainWindow>
#include <QPaintEvent>
#include <QResizeEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainApp : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainApp(QWidget *parent = nullptr);
    ~MainApp();
    void testPacket();
    void pipe_test();
    void moveBackground();

private:
    Ui::MainWindow *ui;
    void paintEvent(QPaintEvent *event) override;
    meshtastic_handler* meshHandler;
    void updateConnectionStatusDisplay();
    QTimer *timer;
    //void moveBackground();

signals:

private slots:
    void on_pushButton_clicked();
    void onConnectionStateChanged(meshtastic_handler::Connection_Status status);
    void onPacketReceived(const QJsonObject& packet);

    void on_packet_view_updateRequest(const QRect &rect, int dy);

    //raw data recvived
    void onRawDataReceived(const QString& rawData);
};

#endif // MAINAPP_H
