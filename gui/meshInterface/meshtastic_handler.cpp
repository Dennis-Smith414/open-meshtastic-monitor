#include "meshtastic_handler.h"
#include "debug_config.h"
#include <QFile>

meshtastic_handler::meshtastic_handler(QObject* parent)
    : QObject(parent), currentState(Disconnected), msgCount(0)
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

    if (serialPort->isOpen()) {
        WARNING_PRINT("Serial port already open, aborting connection attempt");
        return;
    }

    DEBUG_CONNECTION("Setting state to Connecting");
    currentState = Connecting;
    DEBUG_CONNECTION("State changed signal emitted: Connecting");

    QString port = portName;
    if (port.isEmpty()) {
        DEBUG_CONNECTION("No port specified, attempting to find Meshtastic port");
        port = findMeshtasticPort();
        if (port.isEmpty()) {
            ERROR_PRINT("No Meshtastic device found during auto-detection");
            currentState = Error;
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
        DEBUG_CONNECTION("State changed to Connected, signals emitted");
    } else {
        ERROR_PRINT("Failed to open serial port");
        ERROR_PRINT("Error string:" << serialPort->errorString());
        ERROR_PRINT("Error code:" << serialPort->error());
        currentState = Error;
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
                //Cheap and easy way to get this information out of the packet
                //[info] FULL PACKETDEBUG | ??:??:?? 9047 [Router] handleReceived(REMOTE) (id=0x3be6f95d fr=0x433e45e0 to=0xffffffff, WantAck=0, HopLim=3 Ch=0x0 Portnum=67 rxSNR=6 rxRSSI=-21 hopStart=3 relay=0xe0)

            }

            if (logLine.contains("Received text msg")) {
                parseTextData(logLine);
            }

            //turn on debug logs
            if (get_debug_status()){
                emit logMessage("DEBUG: " + logLine);
                qDebug() << "Debug checkbox activated";
            }
        }
    }
}

void meshtastic_handler::parseTextData(QString logLine) {
    DEBUG_PACKET("parseTextData called with:" << logLine);
    QJsonObject textData;

    QRegularExpression textMsgRegex("Received text msg from=0x([a-fA-F0-9]+), id=0x([a-fA-F0-9]+), msg=(.+)$");
    QRegularExpressionMatch match = textMsgRegex.match(logLine);
    if (match.hasMatch()) {
        textData["fromId"] = match.captured(1);
        textData["messageId"] = match.captured(2);
        QString cleanText = match.captured(3);
        cleanText = cleanText.remove('\r').remove('\n').trimmed();
        textData["text"] = cleanText;
        QDateTime currentTime = QDateTime::currentDateTime();
        textData["timestamp"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");
        textData["timestampMs"] = currentTime.toMSecsSinceEpoch();

        DEBUG_PACKET("Parsed text message - From:" << match.captured(1) << "Text:" << cleanText);
    }

    QString textDataString = QJsonDocument(textData).toJson(QJsonDocument::Compact);
    emit logMessage(textDataString);
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

        emit logMessage(batteryInfo);
        DEBUG_PACKET("Parsed battery data - Voltage:" << match.captured(3) << "mV, Percent:" << match.captured(4) << "%");
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


