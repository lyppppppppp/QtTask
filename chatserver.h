#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QObject>
#include <QMap>
#include <QString>
#include "serverworker.h"
#include "database.h"

class ChatServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ChatServer(Database *db, QObject *parent = nullptr);
    ~ChatServer();

    void stopServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void logMessage(const QString &msg);
    void userConnected(const QString &username);
    void userDisconnected(const QString &username);

public slots:
    void jsonReceived(ServerWorker *sender, const QJsonObject &docObj);
    void onUserDisconnected(ServerWorker *sender);

private:
    void broadcastToAll(const QJsonObject &message, ServerWorker *exclude = nullptr);
    void sendToUser(const QString &username, const QJsonObject &message);
    void sendToGroup(const QString &groupName, const QJsonObject &message, ServerWorker *exclude = nullptr);

    QMap<QString, ServerWorker*> m_clients;  // username -> worker
    Database *m_database;
};

#endif // CHATSERVER_H
