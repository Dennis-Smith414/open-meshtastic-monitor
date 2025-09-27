#ifndef MESHTASTIC_HANDLER_H
#define MESHTASTIC_HANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>
#include "debug_config.h"


//protobuf defines
#include "meshtastic/mesh.pb.h"
#include "meshtastic/portnums.pb.h"
#include "meshtastic/telemetry.pb.h"

class meshtastic_handler : public QObject
{
    Q_OBJECT

public:
    enum Connection_Status {
        Disconnected,
        Connecting,
        Connected,
        Error
    };
    Q_ENUM(Connection_Status)

    explicit meshtastic_handler(QObject* parent = nullptr);
    ~meshtastic_handler();

    Connection_Status state() const;
    bool isRunning() const;
    int messageCount() const;

    bool get_debug_status() const {
        return debug_status;
    }

    void set_debug_status(bool status) {
        debug_status = status;
    }


public slots:
    void startMeshtastic(const QString& portName = "");
    void stopMeshtastic();
    void parseTextData(QString logLine);
    void parseBatteryData(QString logLine);
    void parseSenderData(QString logLine);
    void parseHandleReceivedData(QString logLine);

signals:
    void stateChanged(Connection_Status state);
    // void packetReceived(const QJsonObject& packet);sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
    void logMessage(const QString& message, const QString& level = "info");
    void errorOccurred(const QString& error);
   // void rawDataReceived(const QString&(const QString& msg);
    void logBattery(const QString& msg);
    void logNodesOnline(const QString& num_nodes);
    void positionUpdate(const QString& nodeId, double lat, double lon);

private slots:
    void onSerialDataReady();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    QString findMeshtasticPort();
    void processData(const QByteArray& data);
    void processLine(const QString& line);

    QJsonObject parseMessage(const QString& line);
    QSerialPort* serialPort;
    Connection_Status currentState;
    int msgCount;
    QByteArray dataBuffer;
    void processProtobufPacket(const meshtastic::MeshPacket& packet);
    bool debug_status;
    QMap<QString, QJsonObject> pendingPackets;
    QString getPortnumString(int portnum);
    void parsePositionData(QString logLine);
    void parseUpdatePosition(QString logLine);
    void parseNodeStatus(QString logLine);
    int prev_battery_status;
    int cur_battery_status;
    int prev_nodes_num;
    int cur_nodes_num;
};



#endif // MESHTASTIC_HANDLER_H
