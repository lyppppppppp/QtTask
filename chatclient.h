#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

class ChatClient : public QObject
{
    Q_OBJECT

public:
    explicit ChatClient(QObject *parent = nullptr);
    ~ChatClient();

    void connectToServer(const QHostAddress &address, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    QString getUsername() const { return m_username; }

signals:
    void connected();
    void disconnected();
    void jsonReceived(const QJsonObject &docObj);
    void error(const QString &errorString);

public slots:
    void sendJson(const QJsonObject &json);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_clientSocket;
    QString m_username;
    QByteArray m_buffer;
};

#endif // CHATCLIENT_H
