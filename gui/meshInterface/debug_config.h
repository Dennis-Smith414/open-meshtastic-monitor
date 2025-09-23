#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#include <QDebug>
#include <QString>

class debug_config
{
public:
    debug_config();
};

#define ENABLE_DEBUG_PRINT false
#define ENABLE_DEBUG_SERIAL false
#define ENABLE_DEBUG_PACKET false
#define ENABLE_DEBUG_CONNECTION true
#define ENABLE_DEBUG_MESH false

#if ENABLE_DEBUG_PRINT
#define DEBUG_PRINT(msg) qDebug() << msg
#else
#define DEBUG_PRINT(msg) do {} while(0)
#endif

#if ENABLE_DEBUG_SERIAL
#define DEBUG_SERIAL(msg) qDebug() << "[SERIAL]" << msg
#else
#define DEBUG_SERIAL(msg) do {} while(0)
#endif

#if ENABLE_DEBUG_PACKET
#define DEBUG_PACKET(msg) qDebug() << "[PACKET]" << msg
#else
#define DEBUG_PACKET(msg) do {} while(0)
#endif

#if ENABLE_DEBUG_CONNECTION
#define DEBUG_CONNECTION(msg) qDebug() << "[CONNECTION]" << msg
#else
#define DEBUG_CONNECTION(msg) do {} while(0)
#endif

#if ENABLE_DEBUG_MESH
#define DEBUG_MESH(msg) qDebug() << "[MESH]" << msg
#else
#define DEBUG_MESH(msg) do {} while(0)
#endif

#define ERROR_PRINT(msg) qDebug() << "[ERROR]" << msg
#define WARNING_PRINT(msg) qDebug() << "[WARNING]" << msg

#endif // DEBUG_CONFIG_H
