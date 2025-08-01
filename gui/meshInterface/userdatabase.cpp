#include "userdatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

userdatabase::userdatabase(QObject *parent)
    : QObject{parent}
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + "/meshmonitor_users.db";
    db.setDatabaseName(dbPath);
    //Delete at somepoint
    qDebug() << "Database path:" << dbPath;
}

userdatabase::~userdatabase() {
    if(db.isOpen()) {
        db.close();
    }
}

bool userdatabase::initDatabase() {
    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        emit databaseError("Failed to open database: " + db.lastError().text());
        return false;
    }
    return createTables();
}

bool userdatabase::createTables() {
    QSqlQuery query(db);
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL
        )
    )";
    if (!query.exec(createUsersTable)) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
        emit databaseError("Failed to create users table: " + query.lastError().text());
        return false;
    }
    qDebug() << "Database initialized successfully";
    return true;
}

bool userdatabase::addUser(const QString &username, const QString &password) {
    if (userExists(username)) {
        qDebug() << "User already exists:" << username;
        return false;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password_hash) VALUES (:username, :password_hash)");
    query.bindValue(":username", username);
    query.bindValue(":password_hash", hashPassword(password));

    if (!query.exec()) {
        qDebug() << "Failed to add user:" << query.lastError().text();
        emit databaseError("Failed to add user: " + query.lastError().text());
        return false;
    }

    emit userAdded(username);
    qDebug() << "User added successfully:" << username;
    return true;
}

bool userdatabase::authenticateUser(const QString &username, const QString &password) {
    QSqlQuery query(db);
    query.prepare("SELECT password_hash FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qDebug() << "Authentication query failed:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        QString storedHash = query.value("password_hash").toString();
        return verifyPassword(password, storedHash);
    }

    qDebug() << "User not found:" << username;
    return false;
}

bool userdatabase::updateUser(const QString &username, const QString &password) {
    QSqlQuery query(db);
    query.prepare("UPDATE users SET password_hash = :password_hash WHERE username = :username");
    query.bindValue(":username", username);
    query.bindValue(":password_hash", hashPassword(password));

    if (!query.exec()) {
        qDebug() << "Failed to update user:" << query.lastError().text();
        emit databaseError("Failed to update user: " + query.lastError().text());
        return false;
    }

    emit userUpdated(username);
    return query.numRowsAffected() > 0;
}

bool userdatabase::deleteUser(const QString &username, const QString &password) {
    // First verify the password before deleting
    if (!authenticateUser(username, password)) {
        qDebug() << "Authentication failed for delete operation";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qDebug() << "Failed to delete user:" << query.lastError().text();
        emit databaseError("Failed to delete user: " + query.lastError().text());
        return false;
    }

    emit userDeleted(username);
    return query.numRowsAffected() > 0;
}

bool userdatabase::changePassword(const QString &username, const QString &password, const QString &newPassword) {
    // First verify the current password
    if (!authenticateUser(username, password)) {
        qDebug() << "Current password verification failed";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE users SET password_hash = :password_hash WHERE username = :username");
    query.bindValue(":username", username);
    query.bindValue(":password_hash", hashPassword(newPassword));

    if (!query.exec()) {
        qDebug() << "Failed to change password:" << query.lastError().text();
        emit databaseError("Failed to change password: " + query.lastError().text());
        return false;
    }

    emit userUpdated(username);
    return query.numRowsAffected() > 0;
}

User userdatabase::getUser(const QString &username) {
    User user;
    QSqlQuery query(db);
    query.prepare("SELECT username, password_hash FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        user.username = query.value("username").toString();
        user.passwordHash = query.value("password_hash").toString();
    }

    return user;
}

QList<User> userdatabase::getAllUsers() {
    QList<User> users;
    QSqlQuery query(db);

    if (query.exec("SELECT username, password_hash FROM users")) {
        while (query.next()) {
            User user;
            user.username = query.value("username").toString();
            user.passwordHash = query.value("password_hash").toString();
            users.append(user);
        }
    }

    return users;
}

bool userdatabase::userExists(const QString &username) {
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

QString userdatabase::hashPassword(const QString &password) {
    // Generate a random salt
    QString salt = generateSalt();

    // Combine password and salt, then hash with SHA-256
    QString combined = password + salt;
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);

    // Return salt + hash (first 32 chars are salt, rest is hash)
    return salt + hash.toHex();
}

bool userdatabase::verifyPassword(const QString &password, const QString &storedHash) {
    if (storedHash.length() < 32) {
        return false; // Invalid hash format
    }

    // Extract salt (first 32 characters)
    QString salt = storedHash.left(32);
    QString hash = storedHash.mid(32);

    // Hash the provided password with the same salt
    QString combined = password + salt;
    QByteArray newHash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);

    return hash == newHash.toHex();
}

QString userdatabase::generateSalt() {
    QString salt;
    for (int i = 0; i < 32; ++i) {
        salt += QString::number(QRandomGenerator::global()->bounded(16), 16);
    }
    return salt;
}




