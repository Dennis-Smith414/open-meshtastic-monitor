#include "meshtastic_handler.h"
#include <QFile>

meshtastic_handler::meshtastic_handler(QObject* parent)
    : QObject(parent), currentState(Disconnected), msgCount(0)
{
    qDebug() << "[MESHTASTIC] Constructor: Initializing meshtastic_handler";
    serialPort = new QSerialPort(this);

    qDebug() << "[MESHTASTIC] Constructor: Connecting serial port signals";
    connect(serialPort, &QSerialPort::readyRead, this, &meshtastic_handler::onSerialDataReady);
    connect(serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &meshtastic_handler::onSerialError);

    qDebug() << "[MESHTASTIC] Constructor: Initialization complete, state:" << currentState;
}

meshtastic_handler::~meshtastic_handler()
{
    qDebug() << "[MESHTASTIC] Destructor: Cleaning up meshtastic_handler";
    stopMeshtastic();
    qDebug() << "[MESHTASTIC] Destructor: Cleanup complete";
}

meshtastic_handler::Connection_Status meshtastic_handler::state() const
{
    qDebug() << "[MESHTASTIC] State requested, current state:" << currentState;
    return currentState;
}

bool meshtastic_handler::isRunning() const
{
    bool running = serialPort && serialPort->isOpen();
    qDebug() << "[MESHTASTIC] isRunning() called, result:" << running
             << "serialPort exists:" << (serialPort != nullptr)
             << "port open:" << (serialPort ? serialPort->isOpen() : false);
    return running;
}

int meshtastic_handler::messageCount() const
{
    qDebug() << "[MESHTASTIC] messageCount() called, current count:" << msgCount;
    return msgCount;
}

void meshtastic_handler::startMeshtastic(const QString& portName)
{
    qDebug() << "[MESHTASTIC] startMeshtastic() called with portName:" << portName;
    qDebug() << "[MESHTASTIC] Current serial port state - isOpen:" << serialPort->isOpen();

    if (serialPort->isOpen()) {
        qDebug() << "[MESHTASTIC] WARNING: Serial port already open, aborting connection attempt";
        return;
    }

    qDebug() << "[MESHTASTIC] Setting state to Connecting";
    currentState = Connecting;
    emit stateChanged(currentState);
    qDebug() << "[MESHTASTIC] State changed signal emitted: Connecting";

    QString port = portName;
    if (port.isEmpty()) {
        qDebug() << "[MESHTASTIC] No port specified, attempting to find Meshtastic port";
        port = findMeshtasticPort();
        if (port.isEmpty()) {
            qDebug() << "[MESHTASTIC] ERROR: No Meshtastic device found during auto-detection";
            currentState = Error;
            emit stateChanged(currentState);
            emit errorOccurred("No Meshtastic device found");
            qDebug() << "[MESHTASTIC] Error state set and signals emitted";
            return;
        }
        qDebug() << "[MESHTASTIC] Auto-detected port:" << port;
    } else {
        qDebug() << "[MESHTASTIC] Using specified port:" << port;
    }

    emit logMessage("Connecting to port: " + port, "info");
    qDebug() << "[MESHTASTIC] Setting up serial port configuration for port:" << port;

    serialPort->setPortName(port);
    qDebug() << "[MESHTASTIC] Port name set to:" << serialPort->portName();

    serialPort->setBaudRate(QSerialPort::Baud115200);
    qDebug() << "[MESHTASTIC] Baud rate set to:" << serialPort->baudRate();

    serialPort->setDataBits(QSerialPort::Data8);
    qDebug() << "[MESHTASTIC] Data bits set to:" << serialPort->dataBits();

    serialPort->setParity(QSerialPort::NoParity);
    qDebug() << "[MESHTASTIC] Parity set to:" << serialPort->parity();

    serialPort->setStopBits(QSerialPort::OneStop);
    qDebug() << "[MESHTASTIC] Stop bits set to:" << serialPort->stopBits();

    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    qDebug() << "[MESHTASTIC] Flow control set to:" << serialPort->flowControl();

    qDebug() << "[MESHTASTIC] Attempting to open serial port in ReadWrite mode";
    if (serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "[MESHTASTIC] SUCCESS: Serial port opened successfully";
        qDebug() << "[MESHTASTIC] Port details - Name:" << serialPort->portName()
                 << "Baud:" << serialPort->baudRate()
                 << "IsOpen:" << serialPort->isOpen();

        currentState = Connected;
        emit stateChanged(currentState);
        emit logMessage("Connected to Meshtastic device!", "success");
        qDebug() << "[MESHTASTIC] State changed to Connected, signals emitted";
    } else {
        qDebug() << "[MESHTASTIC] ERROR: Failed to open serial port";
        qDebug() << "[MESHTASTIC] Error string:" << serialPort->errorString();
        qDebug() << "[MESHTASTIC] Error code:" << serialPort->error();

        currentState = Error;
        emit stateChanged(currentState);
        emit errorOccurred("Failed to open serial port: " + serialPort->errorString());
        qDebug() << "[MESHTASTIC] Error state set and signals emitted";
    }
}

void meshtastic_handler::stopMeshtastic()
{
    qDebug() << "[MESHTASTIC] stopMeshtastic() called";
    qDebug() << "[MESHTASTIC] Serial port exists:" << (serialPort != nullptr);

    if (serialPort && serialPort->isOpen()) {
        qDebug() << "[MESHTASTIC] Serial port is open, closing connection";
        qDebug() << "[MESHTASTIC] Port name before close:" << serialPort->portName();

        serialPort->close();
        qDebug() << "[MESHTASTIC] Serial port closed, isOpen:" << serialPort->isOpen();

        currentState = Disconnected;
        emit stateChanged(currentState);
        emit logMessage("Disconnected from Meshtastic device", "info");
        qDebug() << "[MESHTASTIC] State changed to Disconnected, signals emitted";
    } else {
        qDebug() << "[MESHTASTIC] Serial port not open or doesn't exist, no action needed";
    }
}

QString meshtastic_handler::findMeshtasticPort()
{
    qDebug() << "[MESHTASTIC] findMeshtasticPort() - Starting port scan";
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    qDebug() << "[MESHTASTIC] Found" << ports.size() << "total serial ports";

    emit logMessage("Scanning for serial ports...", "info");

    for (const QSerialPortInfo& port : ports) {
        qDebug() << "[MESHTASTIC] Examining port:" << port.portName();
        qDebug() << "[MESHTASTIC]   Description:" << port.description();
        qDebug() << "[MESHTASTIC]   Manufacturer:" << port.manufacturer();
        qDebug() << "[MESHTASTIC]   Serial number:" << port.serialNumber();
        qDebug() << "[MESHTASTIC]   Vendor ID:" << QString::number(port.vendorIdentifier(), 16);
        qDebug() << "[MESHTASTIC]   Product ID:" << QString::number(port.productIdentifier(), 16);

        // Look for common Meshtastic device identifiers
        QString desc = port.description().toLower();
        QString mfg = port.manufacturer().toLower();

        qDebug() << "[MESHTASTIC] Checking identifiers - desc (lower):" << desc << "mfg (lower):" << mfg;

        if (desc.contains("usb") ||
            desc.contains("serial") ||
            mfg.contains("silicon labs") ||
            mfg.contains("espressif") ||
            port.portName().contains("USB")) {

            qDebug() << "[MESHTASTIC] MATCH FOUND! Port matches Meshtastic criteria:" << port.portName();
            emit logMessage("Found potential Meshtastic device: " + port.portName() + " (" + port.description() + ")", "info");
            return port.portName();
        }
    }

    qDebug() << "[MESHTASTIC] No specific match found, looking for any USB port";
    for (const QSerialPortInfo& port : ports) {
        qDebug() << "[MESHTASTIC] Checking if port is USB:" << port.portName();
        if (port.portName().contains("USB")) {
            qDebug() << "[MESHTASTIC] Using first available USB port:" << port.portName();
            emit logMessage("Using first USB port: " + port.portName(), "info");
            return port.portName();
        }
    }

    qDebug() << "[MESHTASTIC] No suitable ports found, returning empty string";
    return QString();
}

void meshtastic_handler::onSerialDataReady()
{
    qDebug() << "[MESHTASTIC] onSerialDataReady() - Serial data available";
    qDebug() << "[MESHTASTIC] Bytes available:" << serialPort->bytesAvailable();

    QByteArray data = serialPort->readAll();
    qDebug() << "[MESHTASTIC] Read" << data.size() << "bytes from serial port";
    qDebug() << "[MESHTASTIC] Raw serial data (hex):" << data.toHex(' ');
    qDebug() << "[MESHTASTIC] Raw serial data (string):" << QString::fromUtf8(data);

    processData(data);
    qDebug() << "[MESHTASTIC] onSerialDataReady() complete";
}

void meshtastic_handler::onSerialError(QSerialPort::SerialPortError error)
{
    qDebug() << "[MESHTASTIC] onSerialError() called with error code:" << error;
    qDebug() << "[MESHTASTIC] Error string:" << serialPort->errorString();

    if (error != QSerialPort::NoError) {
        qDebug() << "[MESHTASTIC] Processing serial error, changing state to Error";
        currentState = Error;
        emit stateChanged(currentState);
        emit errorOccurred("Serial port error: " + serialPort->errorString());
        qDebug() << "[MESHTASTIC] Error state set and signals emitted";
    } else {
        qDebug() << "[MESHTASTIC] No error condition, ignoring";
    }
}

// void meshtastic_handler::processProtobufPacket(const meshtastic::MeshPacket& packet)
// {
//     qDebug() << "[MESHTASTIC] Processing protobuf packet";
//     qDebug() << "[MESHTASTIC] From:" << QString::number(packet.from(), 16);
//     qDebug() << "[MESHTASTIC] To:" << QString::number(packet.to(), 16);



//     QJsonObject jsonPacket;
//     jsonPacket["fromID"] = QString::number(packet.from(), 16);
//     jsonPacket["toID"] = QString::number(packet.to(), 16);
//     jsonPacket["id"] = static_cast<qint64>(packet.id());

//     // if (packet.has_rx_rssi()) {
//     //     jsonPacket["rxRssi"] = packet.rx_rssi();s
//     // }

//     if (packet.has_decoded()) {
//         QJsonObject decoded;
//         const auto& data = packet.decoded();

//         decoded["portnum"] = static_cast<int>(data.portnum());

//         if (data.portnum() == meshtastic::PortNum::TEXT_MESSAGE_APP) {
//             decoded["text"] = QString::fromUtf8(data.payload().c_str());
//         } else {
//             decoded["text"] = QJsonValue::Null;
//         }

//         if (data.has_bitfield()) {
//          //   decoded["bitfield"] = data.bitfield();
//         }

//         decoded["latitude"] = QJsonValue::Null;
//         decoded["longitude"] = QJsonValue::Null;
//         decoded["altitude"] = QJsonValue::Null;
//         decoded["batteryLevel"] = QJsonValue::Null;

//         if (data.portnum() == meshtastic::PortNum::POSITION_APP) {
//             // Parse position data
//             meshtastic::Position pos;
//             if (pos.ParseFromString(data.payload())) {
//                 if (pos.has_latitude_i()) {
//                     decoded["latitude"] = pos.latitude_i() / 1e7;
//                 }
//                 if (pos.has_longitude_i()) {
//                     decoded["longitude"] = pos.longitude_i() / 1e7;
//                 }
//                 if (pos.has_altitude()) {
//                     decoded["altitude"] = pos.altitude();
//                 }
//             }
//         }

//         if (data.portnum() == meshtastic::PortNum::TELEMETRY_APP) {
//             meshtastic::Telemetry telemetry;
//             if (telemetry.ParseFromString(data.payload())) {
//                 if (telemetry.has_device_metrics() && telemetry.device_metrics().has_battery_level()) {
//                     decoded["batteryLevel"] = static_cast<qint64>(telemetry.device_metrics().battery_level());
//                 }
//             }
//         }

//     }

//     //avoid flood on un needed packets
//     if (packet.from() != 0 || packet.to() != 0) {
//         emit packetReceived(jsonPacket);
//     }
//     return;
//     //emit packetReceived(jsonPacket);
// }

// void meshtastic_handler::processData(const QByteArray& data) {
//     qDebug() << "[MESHTASTIC] processData() called with" << data.size() << "bytes";
//     dataBuffer.append(data);

//     // Look for protobuf packet markers
//     while (dataBuffer.size() >= 4) { // Minimum packet size
//         // Try to parse as protobuf
//         meshtastic::MeshPacket packet;

//         //need to handle framing here maybe
//         // - Length prefix
//         // - Magic bytes
//         // - Or other framing mechanisms

//         if (packet.ParseFromArray(dataBuffer.data(), dataBuffer.size())) {
//             qDebug() << "[MESHTASTIC] Successfully parsed protobuf packet!";
//             processProtobufPacket(packet);
//             msgCount++;
//             // need to determine how much data was consumed
//             dataBuffer.clear();
//             break;
//         } else {
//             // Remove one byte and try again
//             dataBuffer.remove(0, 1);
//         }
//     }
// }

QJsonObject meshtastic_handler::parseDebugPacket(const QString& logLine) {
    QJsonObject packet;

    // Look for lines that contain packet info - expand the keywords
    if (!logLine.contains("decoded message") &&
        !logLine.contains("handleReceived") &&
        !logLine.contains("Routing sniffing") &&
        !logLine.contains("Delivering rx packet") &&
        !logLine.contains("Forwarding to phone")) {
        return packet;
    }

    // Extract using more flexible regex patterns
    QRegularExpression idRegex("id=0x([a-fA-F0-9]+)");
    QRegularExpressionMatch idMatch = idRegex.match(logLine);
    if (idMatch.hasMatch()) {
        packet["id"] = idMatch.captured(1);
    }

    QRegularExpression fromRegex("fr=0x([a-fA-F0-9]+)");
    QRegularExpressionMatch fromMatch = fromRegex.match(logLine);
    if (fromMatch.hasMatch()) {
        packet["fromId"] = fromMatch.captured(1);
    }

    QRegularExpression toRegex("to=0x([a-fA-F0-9]+)");
    QRegularExpressionMatch toMatch = toRegex.match(logLine);
    if (toMatch.hasMatch()) {
        packet["toId"] = toMatch.captured(1);
    }

    QRegularExpression rssiRegex("rxRSSI=(-?\\d+)");
    QRegularExpressionMatch rssiMatch = rssiRegex.match(logLine);
    if (rssiMatch.hasMatch()) {
        packet["rxRssi"] = rssiMatch.captured(1).toInt();
    }

    QRegularExpression snrRegex("rxSNR=([\\d\\.]+)");
    QRegularExpressionMatch snrMatch = snrRegex.match(logLine);
    if (snrMatch.hasMatch()) {
        packet["rxSnr"] = snrMatch.captured(1).toDouble();
    }

    QRegularExpression hopRegex("HopLim=([\\d]+)");
    QRegularExpressionMatch hopMatch = hopRegex.match(logLine);
    if (hopMatch.hasMatch()) {
        packet["hopLimit"] = hopMatch.captured(1).toInt();
    }

    QRegularExpression portRegex("Portnum=([\\d]+)");
    QRegularExpressionMatch portMatch = portRegex.match(logLine);
    if (portMatch.hasMatch()) {
        QJsonObject decoded;
        decoded["portnum"] = portMatch.captured(1).toInt();
        packet["decoded"] = decoded;
    }

    return packet;
}

void meshtastic_handler::processData(const QByteArray& data) {
    qDebug() << "[MESHTASTIC] processData() called with" << data.size() << "bytes";
    dataBuffer.append(data);
    qDebug() << "[MESHTASTIC] Data buffer size:" << dataBuffer.size();

    QString packet;

    // Process the buffer while there's enough data
    while (dataBuffer.size() >= 4) { // Minimum size for magic bytes + length
        // Check for Meshtastic Protobuf packet (magic bytes: 0x94 0xC3)
        if (dataBuffer.size() >= 4 &&
            static_cast<unsigned char>(dataBuffer[0]) == 0x94 &&
            static_cast<unsigned char>(dataBuffer[1]) == 0xC3) {
            // Read 2-byte length prefix (big-endian)
            quint16 packetLength = (static_cast<unsigned char>(dataBuffer[2]) << 8) |
                                   static_cast<unsigned char>(dataBuffer[3]);

            qDebug() << "[MESHTASTIC] Detected Protobuf packet with length:" << packetLength;
            // Check if we have the full packet (magic bytes + length + payload)

            // Check if we have the full packet (magic bytes + length + payload)
            if (dataBuffer.size() < packetLength + 4) {
                qDebug() << "[MESHTASTIC] Incomplete Protobuf packet, waiting for more data";
                return;
            }

            // Extract and parse the Protobuf packet
            QByteArray packetData = dataBuffer.mid(4, packetLength);
            dataBuffer.remove(0, packetLength + 4); // Remove processed data

            meshtastic::MeshPacket packet;
            if (packet.ParseFromArray(packetData.constData(), packetData.size())) {
                qDebug() << "[MESHTASTIC] Successfully parsed Protobuf packet!";
              //  processProtobufPacket(packet);
                msgCount++;
                qDebug() << "[MESHTASTIC] Message count incremented to:" << msgCount;
            } else {
                qDebug() << "[MESHTASTIC] Failed to parse Protobuf packet";
                emit logMessage("Failed to parse Protobuf packet", "error");
            }
        } else {
            // Assume data is debug/log output (ASCII text or ANSI codes)
            qDebug() << "[MESHTASTIC] Processing as debug/log data";
            // Find the end of a line (e.g., \r\n or \n)
            int lineEnd = dataBuffer.indexOf('\n');
            if (lineEnd == -1) {
                qDebug() << "[MESHTASTIC] No complete debug line, waiting for more data";
                return;
            }

            // Extract the line
            QByteArray logData = dataBuffer.left(lineEnd + 1);
            dataBuffer.remove(0, lineEnd + 1);

            // Clean ANSI escape codes
            QString logLine = QString::fromUtf8(logData);
            QRegularExpression ansiRegex("\x1B\\[[0-9;]*m");
            logLine.remove(ansiRegex);


            QJsonObject debugPacket = parseDebugPacket(logLine);
            if (!debugPacket.isEmpty()) {
                emit packetReceived(debugPacket);
            }

            if (logLine.contains("Battery")) {
                parseBatteryData(logLine);
            }

            if (logLine.contains("Received text msg")) {
                parseTextData(logLine);
            }
            emit logMessage("FULL PACKET" + logLine);
        }
    }
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

void meshtastic_handler::parseTextData(QString logLine) {
    QJsonObject textData;
     QRegularExpression textMsgRegex("Received text msg from=0x([a-fA-F0-9]+), id=0x([a-fA-F0-9]+), msg=(.+)$");
     QRegularExpressionMatch match = textMsgRegex.match(logLine);
     if (match.hasMatch()) {
        textData["fromId"] = match.captured(1);
        textData["messageId"] = match.captured(2);
         QString cleanText = match.captured(3);
         cleanText = cleanText.remove('\r').remove('\n').trimmed();
         textData["text"] = cleanText;
     }
     QString textDataString = QJsonDocument(textData).toJson(QJsonDocument::Compact);
     emit logMessage(textDataString);
}

void meshtastic_handler::parseBatteryData(QString logLine) {
    QJsonObject batteryData;
    QRegularExpression batteryRegex(R"(Battery:\s*usbPower=(\d+),\s*isCharging=(\d+),\s*batMv=(\d+),\s*batPct=(\d+))");
    QRegularExpressionMatch match = batteryRegex.match(logLine);
    if (match.hasMatch()) {
        int usbPower = match.captured(1).toInt();
        int isCharging = match.captured(2).toInt();
        int batteryVoltage = match.captured(3).toInt(); // in millivolts
        int batteryPercent = match.captured(4).toInt();

        // Convert to convenience values
        double voltageVolts = batteryVoltage / 1000.0;
        QString chargingStatus = (isCharging == 1) ? "charging" : "not_charging";
        QString powerSource = (usbPower == 1) ? "usb" : "battery";

        // Create formatted string
        QString batteryInfo = QString("BATTERY: %1%% (%2V) - %3 via %4")
                                  .arg(batteryPercent)
                                  .arg(voltageVolts, 0, 'f', 2)
                                  .arg(chargingStatus)
                                  .arg(powerSource);

        emit logMessage(batteryInfo);

        qDebug() << "[MESHTASTIC] Parsed battery data - Voltage:" << match.captured(3) << "mV, Percent:" << match.captured(4) << "%";
    }

}
