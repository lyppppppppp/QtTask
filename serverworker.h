#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

class ServerWorker : public QObject
{
    Q_OBJECT

public:
    explicit ServerWorker(QObject *parent = nullptr);
    ~ServerWorker();

    bool setSocketDescriptor(qintptr socketDescriptor);
    void disconnectFromClient();
    QString getUsername() const { return m_username; }
    void setUsername(const QString &username) { m_username = username; }

signals:
    void jsonReceived(ServerWorker *sender, const QJsonObject &docObj);
    void disconnectedFromClient();
    void error(QAbstractSocket::SocketError socketError);

public slots:
    void sendJson(const QJsonObject &json);

private slots:
    void receiveJson();

private:
    QTcpSocket *m_clientSocket;
    QString m_username;
    QByteArray m_buffer;
};

#endif // SERVERWORKER_H
