#include "meshtastic_handler.h"
#include "debug_config.h"
#include <QFile>

meshtastic_handler::meshtastic_handler(QObject* parent)
    : QObject(parent), currentState(Disconnected), msgCount(0), debug_status(false)
{
    serialPort = new QSerialPort(this);
    connect(serialPort, &QSerialPort::readyRead, this, &meshtastic_handler::onSerialDataReady);
    connect(serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &meshtastic_handler::onSerialError);
    DEBUG_MESH("meshtastic_handler constructor completed");
}

meshtastic_handler::~meshtastic_handler()
{
    DEBUG_MESH("meshtastic_handler destructor called");
    stopMeshtastic();
}

meshtastic_handler::Connection_Status meshtastic_handler::state() const
{
    DEBUG_SERIAL("Getting current state:" << currentState);
    return currentState;
}

bool meshtastic_handler::isRunning() const
{
    bool running = serialPort && serialPort->isOpen();
    DEBUG_SERIAL("isRunning check - port exists:" << (serialPort != nullptr) << "is open:" << running);
    return running;
}

int meshtastic_handler::messageCount() const
{
    DEBUG_SERIAL("Current message count:" << msgCount);
    return msgCount;
}

void meshtastic_handler::startMeshtastic(const QString& portName)
{
    DEBUG_CONNECTION("startMeshtastic() called with portName:" << portName);
    DEBUG_CONNECTION("Current serial port state - isOpen:" << serialPort->isOpen());

    prev_battery_status = 0;
    cur_battery_status = 0;

    if (serialPort->isOpen()) {
        WARNING_PRINT("Serial port already open, aborting connection attempt");
        return;
    }

    DEBUG_CONNECTION("Setting state to Connecting");
    currentState = Connecting;
    emit stateChanged(currentState);
    DEBUG_CONNECTION("State changed signal emitted: Connecting");



    QString port = portName;
    if (port.isEmpty()) {
        DEBUG_CONNECTION("No port specified, attempting to find Meshtastic port");
        port = findMeshtasticPort();
        if (port.isEmpty()) {
            ERROR_PRINT("No Meshtastic device found during auto-detection");
            currentState = Error;
            emit stateChanged(currentState);
            DEBUG_CONNECTION("Error state set and signals emitted");
            return;
        }
        DEBUG_CONNECTION("Auto-detected port:" << port);
    } else {
        DEBUG_CONNECTION("Using specified port:" << port);
    }
    DEBUG_CONNECTION("Setting up serial port configuration for port:" << port);

    serialPort->setPortName(port);
    DEBUG_SERIAL("Port name set to:" << serialPort->portName());
    serialPort->setBaudRate(QSerialPort::Baud115200);
    DEBUG_SERIAL("Baud rate set to:" << serialPort->baudRate());
    serialPort->setDataBits(QSerialPort::Data8);
    DEBUG_SERIAL("Data bits set to:" << serialPort->dataBits());
    serialPort->setParity(QSerialPort::NoParity);
    DEBUG_SERIAL("Parity set to:" << serialPort->parity());
    serialPort->setStopBits(QSerialPort::OneStop);
    DEBUG_SERIAL("Stop bits set to:" << serialPort->stopBits());
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    DEBUG_SERIAL("Flow control set to:" << serialPort->flowControl());

    DEBUG_CONNECTION("Attempting to open serial port in ReadWrite mode");
    if (serialPort->open(QIODevice::ReadWrite)) {
        DEBUG_CONNECTION("SUCCESS: Serial port opened successfully");
        DEBUG_CONNECTION("Port details - Name:" << serialPort->portName()
                                                << "Baud:" << serialPort->baudRate()
                                                << "IsOpen:" << serialPort->isOpen());
        currentState = Connected;
        emit stateChanged(currentState);
        DEBUG_CONNECTION("State changed to Connected, signals emitted");
    } else {
        ERROR_PRINT("Failed to open serial port");
        ERROR_PRINT("Error string:" << serialPort->errorString());
        ERROR_PRINT("Error code:" << serialPort->error());
        currentState = Error;
        emit stateChanged(currentState);
        DEBUG_CONNECTION("Error state set and signals emitted");
    }
}

void meshtastic_handler::stopMeshtastic()
{
    DEBUG_CONNECTION("stopMeshtastic() called");
    DEBUG_CONNECTION("Serial port exists:" << (serialPort != nullptr));

    if (serialPort && serialPort->isOpen()) {
        DEBUG_CONNECTION("Serial port is open, closing connection");
        DEBUG_CONNECTION("Port name before close:" << serialPort->portName());
        serialPort->close();
        DEBUG_SERIAL("Serial port closed, isOpen:" << serialPort->isOpen());
        currentState = Disconnected;
        DEBUG_CONNECTION("State changed to Disconnected, signals emitted");
    } else {
        DEBUG_CONNECTION("Serial port not open or doesn't exist, no action needed");
    }
}

QString meshtastic_handler::findMeshtasticPort()
{
    DEBUG_CONNECTION("findMeshtasticPort() - Starting port scan");
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    DEBUG_CONNECTION("Found" << ports.size() << "total serial ports");

    for (const QSerialPortInfo& port : ports) {
        DEBUG_CONNECTION("Examining port:" << port.portName());
        DEBUG_CONNECTION("  Description:" << port.description());
        DEBUG_CONNECTION("  Manufacturer:" << port.manufacturer());
        DEBUG_CONNECTION("  Serial number:" << port.serialNumber());
        DEBUG_CONNECTION("  Vendor ID:" << QString::number(port.vendorIdentifier(), 16));
        DEBUG_CONNECTION("  Product ID:" << QString::number(port.productIdentifier(), 16));

        // Look for common Meshtastic device identifiers
        QString desc = port.description().toLower();
        QString mfg = port.manufacturer().toLower();
        DEBUG_CONNECTION("Checking identifiers - desc (lower):" << desc << "mfg (lower):" << mfg);

        if (desc.contains("usb") ||
            desc.contains("serial") ||
            mfg.contains("silicon labs") ||
            mfg.contains("espressif") ||
            port.portName().contains("USB")) {

            DEBUG_CONNECTION("MATCH FOUND! Port matches Meshtastic criteria:" << port.portName());
            return port.portName();
        }
    }

    DEBUG_CONNECTION("No specific match found, looking for any USB port");
    for (const QSerialPortInfo& port : ports) {
        DEBUG_CONNECTION("Checking if port is USB:" << port.portName());
        if (port.portName().contains("USB")) {
            DEBUG_CONNECTION("Using first available USB port:" << port.portName());

            return port.portName();
        }
    }

    ERROR_PRINT("No suitable ports found, returning empty string");
    return QString();
}

void meshtastic_handler::onSerialDataReady()
{
    DEBUG_SERIAL("onSerialDataReady() - Serial data available");
    DEBUG_SERIAL("Bytes available:" << serialPort->bytesAvailable());

    QByteArray data = serialPort->readAll();
    DEBUG_SERIAL("Read" << data.size() << "bytes from serial port");
    DEBUG_SERIAL("Raw serial data (hex):" << data.toHex(' '));
    DEBUG_SERIAL("Raw serial data (string):" << QString::fromUtf8(data));

    processData(data);
    DEBUG_SERIAL("onSerialDataReady() complete");
}

void meshtastic_handler::onSerialError(QSerialPort::SerialPortError error)
{
    DEBUG_SERIAL("onSerialError() called with error code:" << error);
    DEBUG_SERIAL("Error string:" << serialPort->errorString());

    if (error != QSerialPort::NoError) {
        ERROR_PRINT("Processing serial error, changing state to Error");
        currentState = Error;
        DEBUG_CONNECTION("Error state set and signals emitted");
        emit logMessage("Connection Error!");
    } else {
        DEBUG_SERIAL("No error condition, ignoring");
    }
}

void meshtastic_handler::processData(const QByteArray& data) {
    DEBUG_PACKET("processData() called with" << data.size() << "bytes");
    dataBuffer.append(data);
    DEBUG_PACKET("Data buffer size:" << dataBuffer.size());

    // Process the buffer while there's enough data
    while (dataBuffer.size() >= 4) { // Minimum size for magic bytes + length
        // Check for Protobuf packet (magic bytes: 0x94 0xC3)
        if (dataBuffer.size() >= 4 &&
            static_cast<unsigned char>(dataBuffer[0]) == 0x94 &&
            static_cast<unsigned char>(dataBuffer[1]) == 0xC3) {

            // Read 2-byte length prefix (big-endian)
            quint16 packetLength = (static_cast<unsigned char>(dataBuffer[2]) << 8) |
                                   static_cast<unsigned char>(dataBuffer[3]);
            DEBUG_PACKET("Detected Protobuf packet with length:" << packetLength);

            // Check if we have the full packet (magic bytes + length + payload)
            if (dataBuffer.size() < packetLength + 4) {
                DEBUG_PACKET("Incomplete Protobuf packet, waiting for more data");
                return;
            }

            // Extract and parse
            QByteArray packetData = dataBuffer.mid(4, packetLength);
            dataBuffer.remove(0, packetLength + 4); // Remove processed data

            meshtastic::MeshPacket packet;
            if (packet.ParseFromArray(packetData.constData(), packetData.size())) {
                DEBUG_PACKET("Successfully parsed Protobuf packet!");
                // processProtobufPacket(packet);
                msgCount++;
                DEBUG_PACKET("Message count incremented to:" << msgCount);
            } else {
                ERROR_PRINT("Failed to parse Protobuf packet");
                emit logMessage("Failed to parse Protobuf packet", "error");
            }
        } else {
            // Assume data is debug/log output (ASCII text or ANSI codes)
            DEBUG_PACKET("Processing as debug/log data");

            // Find the end of a line (e.g., \r\n or \n)
            int lineEnd = dataBuffer.indexOf('\n');
            if (lineEnd == -1) {
                DEBUG_PACKET("No complete debug line, waiting for more data");
                return;
            }

            // Extract the line
            QByteArray logData = dataBuffer.left(lineEnd + 1);
            dataBuffer.remove(0, lineEnd + 1);

            // Clean ANSI escape codes
            QString logLine = QString::fromUtf8(logData);
            QRegularExpression ansiRegex("\x1B\\[[0-9;]*m");
            logLine.remove(ansiRegex);

            //grab battery information
            //This will be added to a "Server" node view after a check to see if it is different then the prev value made it has to do this
            //a few times to avoid constantly updating values.
            if (logLine.contains("Battery")) {
                parseBatteryData(logLine);
            }

            if (logLine.contains("Received from")) {
                parseSenderData(logLine);
            }

            if (logLine.contains("handleReceived")) {
                parseHandleReceivedData(logLine);
            }

            if (logLine.contains("Received text msg")) {
                parseTextData(logLine);
            }

            if (logLine.contains("updatePosition REMOTE")) {
                parseUpdatePosition(logLine);
            }

            if (logLine.contains("node=")) {
                parsePositionData(logLine);
            }

            if (logLine.contains("Node status update:")) {
                parseNodeStatus(logLine);
            }

            //turn on debug logs
            if (get_debug_status()){
                emit logMessage("DEBUG: " + logLine);
            }
        }
    }
}


QString meshtastic_handler::getPortnumString(int portnum) {
    switch (portnum) {
    case 0: return "UNKNOWN_APP";
    case 1: return "TEXT_MESSAGE_APP";
    case 3: return "POSITION_APP";
    case 4: return "NODEINFO_APP";
    case 6: return "ROUTING_APP";
    case 7: return "ADMIN_APP";
    case 8: return "TEXT_MESSAGE_COMPRESSED_APP";
    case 9: return "WAYPOINT_APP";
    case 64: return "SERIAL_APP";
    case 65: return "STORE_FORWARD_APP";
    case 66: return "RANGE_TEST_APP";
    case 67: return "TELEMETRY_APP";
    case 68: return "ZPS_APP";
    case 69: return "SIMULATOR_APP";
    default: return QString("UNKNOWN_%1").arg(portnum);
    }
}

void meshtastic_handler::parseNodeStatus(QString logLine) {
    DEBUG_PACKET("parseNodeStatus called with:" << logLine);

    QRegularExpression nodeStatusRegex(R"(Node status update:\s*(\d+)\s*online,\s*(\d+)\s*total)");
    QRegularExpressionMatch match = nodeStatusRegex.match(logLine);

    if (match.hasMatch()) {
        int onlineNodes = match.captured(1).toInt();
        int totalNodes = match.captured(2).toInt();

        QJsonObject nodeStatus;
        nodeStatus["onlineNodes"] = onlineNodes;
        nodeStatus["totalNodes"] = totalNodes;
        nodeStatus["type"] = "NODE_STATUS";

        if (prev_nodes_num != cur_nodes_num) {
            QString final_num = QString::number(onlineNodes);
            emit logNodesOnline(final_num);
        }

        QDateTime currentTime = QDateTime::currentDateTime();
        nodeStatus["timestamp"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");

        QString nodeJson = QJsonDocument(nodeStatus).toJson(QJsonDocument::Compact);
        emit logMessage(nodeJson, "nodes");

        prev_nodes_num = cur_nodes_num;

        DEBUG_PACKET("Node Status - Online:" << onlineNodes << "Total:" << totalNodes);
    }
}

void meshtastic_handler::parseHandleReceivedData(QString logLine) {
    DEBUG_PACKET("parseHandleReceivedData called with:" << logLine);

    // Update regex to capture transport field
    QRegularExpression handleReceivedRegex(R"(handleReceived\([^)]*\)\s*\(id=0x([a-fA-F0-9]+)\s+fr=0x([a-fA-F0-9]+)\s+to=0x([a-fA-F0-9]+)[^,]*,\s*transport\s*=\s*(\d+)[^,]*,\s*WantAck=(\d+)[^,]*,\s*HopLim=(\d+)[^,]*Ch=0x([a-fA-F0-9]+)[^,]*Portnum=(\d+)(?:[^,]*rxtime=(\d+))?(?:[^,]*rxSNR=(-?\d+(?:\.\d+)?))?(?:[^,]*rxRSSI=(-?\d+))?(?:[^,]*hopStart=(\d+))?)");

    QRegularExpressionMatch match = handleReceivedRegex.match(logLine);
    if (match.hasMatch()) {
        QString messageId = match.captured(1);

        // Convert to decimal
        bool ok;
        qint64 id = messageId.toULongLong(&ok, 16);
        qint64 from = match.captured(2).toULongLong(&ok, 16);
        qint64 to = match.captured(3).toULongLong(&ok, 16);

        QJsonObject packetData;
        packetData["from"] = from;
        packetData["to"] = to;
        packetData["id"] = id;
        packetData["rxTime"] = match.captured(9).isEmpty() ? QJsonValue(QJsonValue::Null) : match.captured(9).toLongLong();
        packetData["rxSnr"] = match.captured(10).isEmpty() ? QJsonValue(QJsonValue::Null) : match.captured(10).toDouble();
        packetData["rxRssi"] = match.captured(11).isEmpty() ? QJsonValue(QJsonValue::Null) : match.captured(11).toInt();
        packetData["hopLimit"] = match.captured(6).toInt();
        packetData["hopStart"] = match.captured(12).isEmpty() ? 3 : match.captured(12).toInt();
        packetData["fromId"] = QString("!%1").arg(from, 8, 16, QChar('0'));
        packetData["toId"] = (to == 0xffffffff) ? "^all" : QString("!%1").arg(to, 8, 16, QChar('0'));

        int transport = match.captured(4).toInt();
        int portnum = match.captured(8).toInt();

        if (portnum == 1) {
            // TEXT MESSAGE - store for later merging, don't emit yet
            packetData["transport"] = transport;
            pendingPackets[messageId] = packetData;
            DEBUG_PACKET("Stored TEXT packet for merging with message ID:" << messageId);
        } else {
            // NON-TEXT MESSAGE - emit immediately with decoded section
            QJsonObject decoded;
            decoded["portnum"] = getPortnumString(portnum);
            decoded["text"] = QJsonValue(QJsonValue::Null);
            decoded["bitfield"] = transport;
            decoded["latitude"] = QJsonValue(QJsonValue::Null);
            decoded["longitude"] = QJsonValue(QJsonValue::Null);
            decoded["altitude"] = QJsonValue(QJsonValue::Null);
            decoded["batteryLevel"] = QJsonValue(QJsonValue::Null);

            packetData["decoded"] = decoded;

            QString packetDataString = QJsonDocument(packetData).toJson(QJsonDocument::Compact);
            //emit logMessage(packetDataString, "packet");
        }
    }
}

// void meshtastic_handler::parseTextData(QString logLine) {
//     DEBUG_PACKET("parseTextData called with:" << logLine);
//     QJsonObject textData;

//     QRegularExpression textMsgRegex("Received text msg from=0x([a-fA-F0-9]+), id=0x([a-fA-F0-9]+), msg=(.+)$");
//     QRegularExpressionMatch match = textMsgRegex.match(logLine);
//     if (match.hasMatch()) {
//         textData["fromId"] = match.captured(1);
//         textData["messageId"] = match.captured(2);
//         QString cleanText = match.captured(3);
//         cleanText = cleanText.remove('\r').remove('\n').trimmed();
//         textData["text"] = cleanText;
//         QDateTime currentTime = QDateTime::currentDateTime();
//         textData["timestamp"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");
//         textData["timestampMs"] = currentTime.toMSecsSinceEpoch();

//         DEBUG_PACKET("Parsed text message - From:" << match.captured(1) << "Text:" << cleanText);
//     }

//     QString textDataString = QJsonDocument(textData).toJson(QJsonDocument::Compact);
//     emit logMessage(textDataString);
// }

void meshtastic_handler::parseTextData(QString logLine) {
    DEBUG_PACKET("parseTextData called with:" << logLine);

    QRegularExpression textMsgRegex("Received text msg from=0x([a-fA-F0-9]+), id=0x([a-fA-F0-9]+), msg=(.+)$");
    QRegularExpressionMatch match = textMsgRegex.match(logLine);

    if (match.hasMatch()) {
        QString messageId = match.captured(2);
        QString cleanText = match.captured(3).remove('\r').remove('\n').trimmed();

        // Check if we have stored packet data for this message ID
        if (pendingPackets.contains(messageId)) {
            // Get the stored packet data and remove it from pending
            QJsonObject packetData = pendingPackets.take(messageId);

            // Build the decoded section with text message data
            QJsonObject decoded;
            decoded["portnum"] = "TEXT_MESSAGE_APP";
            decoded["text"] = cleanText;
            decoded["bitfield"] = packetData["transport"].toInt();
            decoded["latitude"] = QJsonValue(QJsonValue::Null);
            decoded["longitude"] = QJsonValue(QJsonValue::Null);
            decoded["altitude"] = QJsonValue(QJsonValue::Null);
            decoded["batteryLevel"] = QJsonValue(QJsonValue::Null);

            // Add decoded section to packet data
            packetData["decoded"] = decoded;

            // Remove the transport field since it's now in bitfield
            packetData.remove("transport");

            // Output the complete merged JSON
            QString finalJson = QJsonDocument(packetData).toJson(QJsonDocument::Compact);
            emit logMessage(finalJson, "packet");

            DEBUG_PACKET("Merged complete packet for message ID:" << messageId << "Text:" << cleanText);

        } else {
            // Fallback: no stored packet data found, output text-only data
            QJsonObject textOnly;
            textOnly["fromId"] = match.captured(1);
            textOnly["messageId"] = messageId;
            textOnly["text"] = cleanText;
            QDateTime currentTime = QDateTime::currentDateTime();
            textOnly["timestamp"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");
            textOnly["timestampMs"] = currentTime.toMSecsSinceEpoch();

            QString textOnlyJson = QJsonDocument(textOnly).toJson(QJsonDocument::Compact);
            emit logMessage(textOnlyJson, "info");

            DEBUG_PACKET("No stored packet data for message ID:" << messageId << ", output text-only");
        }
    }
}

void meshtastic_handler::parseBatteryData(QString logLine) {
    DEBUG_PACKET("parseBatteryData called with:" << logLine);
    QJsonObject batteryData;

    QRegularExpression batteryRegex(R"(Battery:\s*usbPower=(\d+),\s*isCharging=(\d+),\s*batMv=(\d+),\s*batPct=(\d+))");
    QRegularExpressionMatch match = batteryRegex.match(logLine);
    if (match.hasMatch()) {
        int usbPower = match.captured(1).toInt();
        int isCharging = match.captured(2).toInt();
        int batteryVoltage = match.captured(3).toInt();
        int batteryPercent = match.captured(4).toInt();

        cur_battery_status = batteryPercent;

        if (prev_battery_status != cur_battery_status) {
            QString battery_status= QString::number(batteryPercent);
            emit logBattery(battery_status);
        }

        // Convert to more readable values
        double voltageVolts = batteryVoltage / 1000.0;
        QString chargingStatus = (isCharging == 1) ? "charging" : "not_charging";
        QString powerSource = (usbPower == 1) ? "usb" : "battery";

        // Create string
        QString batteryInfo = QString("BATTERY: %1%% (%2V) - %3 via %4")
                                  .arg(batteryPercent)
                                  .arg(voltageVolts, 0, 'f', 2)
                                  .arg(chargingStatus)
                                  .arg(powerSource);

        prev_battery_status = cur_battery_status;

        //emit logMessage(batteryInfo); <-----Commented for now add this to a seperate screen for battery info
        DEBUG_PACKET("Parsed battery data - Voltage:" << match.captured(3) << "mV, Percent:" << match.captured(4) << "%");
    }
}

void meshtastic_handler::parsePositionData(QString logLine) {
    DEBUG_PACKET("parsePositionData called with:" << logLine);

    QRegularExpression positionRegex(R"(POSITION node=([a-fA-F0-9]+)[^=]*lat=(-?\d+)[^=]*lon=(-?\d+)[^=]*msl=(\d+))");
    QRegularExpressionMatch match = positionRegex.match(logLine);

    if (match.hasMatch()) {
        QString nodeId = match.captured(1);
        double latitude = match.captured(2).toDouble() / 10000000.0;
        double longitude = match.captured(3).toDouble() / 10000000.0;
        int altitude = match.captured(4).toInt();

        // Create position JSON object
        QJsonObject positionData;
        positionData["nodeId"] = QString("!%1").arg(nodeId);
        positionData["latitude"] = latitude;
        positionData["longitude"] = longitude;
        positionData["altitude"] = altitude;
        positionData["type"] = "POSITION";

        QDateTime currentTime = QDateTime::currentDateTime();
        positionData["timestamp"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");
        positionData["timestampMs"] = currentTime.toMSecsSinceEpoch();

        QString positionJson = QJsonDocument(positionData).toJson(QJsonDocument::Compact);
        emit logMessage(positionJson, "position");

        DEBUG_PACKET("GPS - Node:" << nodeId << "Lat:" << latitude << "Lon:" << longitude << "Alt:" << altitude);
    }
}

void meshtastic_handler::parseUpdatePosition(QString logLine) {
    DEBUG_PACKET("parseUpdatePosition called with:" << logLine);
    qDebug() << "UPDATE POSITION PARSER CALLED WITH:" << logLine;

    QRegularExpression updatePosRegex(R"(updatePosition\s+REMOTE\s+node=0x([a-fA-F0-9]+)\s+time=(\d+)\s+lat=(-?\d+)\s+lon=(-?\d+))");
    QRegularExpressionMatch match = updatePosRegex.match(logLine);

    if (match.hasMatch()) {
        QString nodeId = match.captured(1);
        double latitude = match.captured(3).toDouble() / 10000000.0;
        double longitude = match.captured(4).toDouble() / 10000000.0;

        QJsonObject positionData;
        positionData["nodeId"] = QString("!%1").arg(nodeId);
        positionData["latitude"] = latitude;
        positionData["longitude"] = longitude;
        positionData["type"] = "POSITION";

        QDateTime currentTime = QDateTime::currentDateTime();
        positionData["timestamp"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");

        QString positionJson = QJsonDocument(positionData).toJson(QJsonDocument::Compact);
        emit logMessage(positionJson, "position");

        DEBUG_PACKET("GPS Update - Node:" << nodeId << "Lat:" << latitude << "Lon:" << longitude);
        qDebug() << "EMITTED POSITION JSON:" << positionJson;
    } else {
        qDebug() << "REGEX DID NOT MATCH updatePosition line";
    }
}

void meshtastic_handler::parseSenderData(QString logLine) {
    QJsonObject senderData;
    QRegularExpression senderRegex(R"(\(Received from ([a-fA-F0-9]+)\): air_util_tx=([0-9.]+), channel_utilization=([0-9.]+), battery_level=(\d+), voltage=([0-9.]+))");
    QRegularExpressionMatch match = senderRegex.match(logLine);

    if (match.hasMatch()) {
        senderData["fromId"] = match.captured(1);
        senderData["airUtilTx"] = match.captured(2).toDouble();
        senderData["channelUtilization"] = match.captured(3).toDouble();
        senderData["batteryLevel"] = match.captured(4).toInt();
        senderData["voltage"] = match.captured(5).toDouble();

        DEBUG_PACKET("Parsed sender data - From:" << match.captured(1)
                                                  << " air_util_tx:" << match.captured(2)
                                                  << " channel_utilization:" << match.captured(3)
                                                  << " battery_level:" << match.captured(4)
                                                  << " voltage:" << match.captured(5));
    } else {
        DEBUG_PACKET("No sender data found!");
        return;
    }
    QString senderDataString = QJsonDocument(senderData).toJson(QJsonDocument::Compact);
    emit logMessage(senderDataString);
}


