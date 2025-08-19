#include "meshtastic_handler.h"

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
    // If no specific match, try the first USB port
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

void meshtastic_handler::processData(const QByteArray& data)
{
    qDebug() << "[MESHTASTIC] processData() called with" << data.size() << "bytes";
    qDebug() << "[MESHTASTIC] Current buffer size before append:" << dataBuffer.size();

    dataBuffer.append(data);
    qDebug() << "[MESHTASTIC] Buffer size after append:" << dataBuffer.size();
    qDebug() << "[MESHTASTIC] Buffer contents:" << QString::fromUtf8(dataBuffer);

    // Process complete lines
    int linesProcessed = 0;
    while (dataBuffer.contains('\n')) {
        qDebug() << "[MESHTASTIC] Found newline in buffer, processing line" << (linesProcessed + 1);

        int lineEnd = dataBuffer.indexOf('\n');
        qDebug() << "[MESHTASTIC] Line end position:" << lineEnd;

        QByteArray lineData = dataBuffer.left(lineEnd);
        qDebug() << "[MESHTASTIC] Extracted line data:" << QString::fromUtf8(lineData);

        dataBuffer.remove(0, lineEnd + 1);
        qDebug() << "[MESHTASTIC] Remaining buffer size:" << dataBuffer.size();

        QString line = QString::fromUtf8(lineData).trimmed();
        qDebug() << "[MESHTASTIC] Trimmed line:" << line;

        if (!line.isEmpty()) {
            qDebug() << "[MESHTASTIC] Processing non-empty line:" << line;
            processLine(line);
        } else {
            qDebug() << "[MESHTASTIC] Skipping empty line";
        }

        linesProcessed++;
    }

    qDebug() << "[MESHTASTIC] processData() complete, processed" << linesProcessed << "lines";
    qDebug() << "[MESHTASTIC] Remaining buffer size:" << dataBuffer.size();
}

void meshtastic_handler::processLine(const QString& line)
{
    qDebug() << "[MESHTASTIC] processLine() called with:" << line;
    qDebug() << "[MESHTASTIC] Line length:" << line.length();
    qDebug() << "[MESHTASTIC] Line starts with '{':" << line.startsWith('{');

    // Look for JSON data (Meshtastic can output JSON via serial)
    if (line.startsWith('{')) {
        qDebug() << "[MESHTASTIC] Detected JSON data, attempting to parse";

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);

        qDebug() << "[MESHTASTIC] JSON parse result - Error:" << error.error;
        qDebug() << "[MESHTASTIC] JSON parse error string:" << error.errorString();
        qDebug() << "[MESHTASTIC] JSON parse error offset:" << error.offset;

        if (error.error == QJsonParseError::NoError) {
            qDebug() << "[MESHTASTIC] JSON parsed successfully!";

            QJsonObject packet = doc.object();
            qDebug() << "[MESHTASTIC] JSON object keys:" << packet.keys();

            msgCount++;
            qDebug() << "[MESHTASTIC] PACKET RECEIVED from serial! Count incremented to:" << msgCount;
            qDebug() << "[MESHTASTIC] Packet content:" << doc.toJson(QJsonDocument::Compact);

            emit packetReceived(packet);
            qDebug() << "[MESHTASTIC] packetReceived signal emitted";
        } else {
            qDebug() << "[MESHTASTIC] JSON Parse Error at offset" << error.offset << ":" << error.errorString();
            qDebug() << "[MESHTASTIC] Problematic JSON:" << line;
            emit logMessage("JSON Parse Error: " + error.errorString(), "error");
        }
    } else {
        qDebug() << "[MESHTASTIC] Non-JSON line received, logging as debug";
        // Log other serial output
        emit logMessage("Serial: " + line, "debug");
        qDebug() << "[MESHTASTIC] Debug message emitted for line:" << line;
    }

    qDebug() << "[MESHTASTIC] processLine() complete";
}
