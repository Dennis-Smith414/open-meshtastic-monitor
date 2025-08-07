#ifndef MESHTASTIC_HANDLER_H
#define MESHTASTIC_HANDLER_H

#include <QObject>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDateTime>
#include <QDebug>


class meshtastic_handler
{
    Q_OBJECT

public:
    meshtastic_handler();

    enum Connection_Status {
        Disconnected,
        Connecting,
        Connected,
        Error,
    };
    Q_ENUM(Connection_Status)
    explicit meshtastic_handler(QObject* parent = nullptr);
    ~meshtastic_handler();

    //State Quries

    //Slots

    //Signals

    //Private slots

    //Util class for help with parcing

    //TODO: Data storeage for packet history

};

#endif // MESHTASTIC_HANDLER_H
