#include "database.h"
#include <QDebug>

Database::Database(QObject *parent)
    : QObject(parent)
{
}

Database::~Database()
{
    closeDatabase();
}

bool Database::initializeDatabase(const QString &dbPath)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "ClientConnection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);

    // 创建消息表
    query.exec("CREATE TABLE IF NOT EXISTS messages ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "sender TEXT NOT NULL,"
               "receiver TEXT NOT NULL,"
               "content TEXT NOT NULL,"
               "message_type TEXT NOT NULL DEFAULT 'private',"
               "group_name TEXT,"
               "timestamp TEXT,"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
               ")");

    // 创建索引
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_target ON messages(receiver, message_type)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_group ON messages(group_name)");

    return true;
}

bool Database::closeDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase("ClientConnection");
    return true;
}

bool Database::saveMessage(const QString &sender, const QString &receiver, const QString &content,
                          const QString &messageType, const QString &groupName, const QString &timestamp)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (sender, receiver, content, message_type, group_name, timestamp) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(sender);
    query.addBindValue(receiver);
    query.addBindValue(content);
    query.addBindValue(messageType);
    query.addBindValue(groupName);
    query.addBindValue(timestamp.isEmpty() ? QDateTime::currentDateTime().toString(Qt::ISODate) : timestamp);

    return query.exec();
}

QJsonArray Database::getMessages(const QString &target, const QString &messageType, 
                                const QString &currentUser, int limit)
{
    QJsonArray messages;
    QSqlQuery query(m_db);

    if (messageType == "private") {
        // 查询所有与target相关的私聊消息（无论是sender还是receiver）
        // currentUser是当前登录用户，target是聊天对象
        query.prepare("SELECT sender, receiver, content, timestamp FROM messages "
                      "WHERE ((sender = ? AND receiver = ?) OR (sender = ? AND receiver = ?)) "
                      "AND message_type = 'private' "
                      "ORDER BY created_at DESC LIMIT ?");
        query.addBindValue(currentUser);  // sender = currentUser, receiver = target
        query.addBindValue(target);
        query.addBindValue(target);        // sender = target, receiver = currentUser
        query.addBindValue(currentUser);
        query.addBindValue(limit);
    } else {
        query.prepare("SELECT sender, receiver, content, timestamp FROM messages "
                      "WHERE group_name = ? AND message_type = 'group' "
                      "ORDER BY created_at DESC LIMIT ?");
        query.addBindValue(target);
        query.addBindValue(limit);
    }

    if (query.exec()) {
        while (query.next()) {
            QJsonObject message;
            message["sender"] = query.value(0).toString();
            message["receiver"] = query.value(1).toString();
            message["content"] = query.value(2).toString();
            message["timestamp"] = query.value(3).toString();
            messages.append(message);
        }
    }

    return messages;
}

bool Database::clearMessages()
{
    QSqlQuery query(m_db);
    return query.exec("DELETE FROM messages");
}
