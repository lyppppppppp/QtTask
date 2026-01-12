#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    bool initializeDatabase(const QString &dbPath);
    bool closeDatabase();

    // 消息管理
    bool saveMessage(const QString &sender, const QString &receiver, const QString &content,
                     const QString &messageType = "private", const QString &groupName = "",
                     const QString &timestamp = "");
    QJsonArray getMessages(const QString &target, const QString &messageType = "private", 
                          const QString &currentUser = "", int limit = 100);
    bool clearMessages();

private:
    QSqlDatabase m_db;
};

#endif // DATABASE_H
