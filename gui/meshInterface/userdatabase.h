#ifndef USERDATABASE_H
#define USERDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QDebug>

struct User {
    QString username;
    QString passwordHash;
};

class userdatabase : public QObject
{
    Q_OBJECT
public:
    explicit userdatabase(QObject *parent = nullptr);
    ~userdatabase();

    //Database Ops
    bool initDatabase();
    bool addUser(const QString &username, const QString &password);
    bool authenticateUser(const QString &username, const QString &passowrd);
    bool updateUser(const QString &username, const QString &password);
    bool deleteUser(const QString &username, const QString &password);
    bool changePassword(const QString &username, const QString &password, const QString &newPassword);

    //Query Ops
    User getUser(const QString &username);
    QList<User> getAllUsers();
    bool userExists(const QString &username);

    //Uil functions
    QString hashPassword(const QString &password);
    bool verifyPassword(const QString &password, const QString &hash);

private:
    QSqlDatabase db;
    bool createTables();
    QString generateSalt();

signals:
    void userAdded(const QString &username);
    void userUpdated(const QString &username);
    void userDeleted(const QString &username);
    void databaseError(const QString &error);
};

#endif // USERDATABASE_H
