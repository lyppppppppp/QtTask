#include "chatclient.h"
#include <QDebug>
#include <QDataStream>

ChatClient::ChatClient(QObject *parent)
    : QObject(parent)
    , m_clientSocket(new QTcpSocket(this))
{
    connect(m_clientSocket, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_clientSocket, &QAbstractSocket::errorOccurred, this, &ChatClient::onError);
#else
    connect(m_clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &ChatClient::onError);
#endif
}

ChatClient::~ChatClient()
{
    disconnectFromServer();
}

void ChatClient::connectToServer(const QHostAddress &address, quint16 port)
{
    // 如果已经连接或正在连接，先断开
    if (m_clientSocket->state() != QAbstractSocket::UnconnectedState) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->waitForDisconnected(1000);
    }
    m_clientSocket->connectToHost(address, port);
}

void ChatClient::disconnectFromServer()
{
    if (m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        m_clientSocket->disconnectFromHost();
    }
}

bool ChatClient::isConnected() const
{
    return m_clientSocket->state() == QAbstractSocket::ConnectedState;
}

void ChatClient::sendJson(const QJsonObject &json)
{
    if (!isConnected()) {
        emit error("未连接到服务器");
        return;
    }

    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream << static_cast<quint32>(data.size());
    packet.append(data);
    m_clientSocket->write(packet);
}

void ChatClient::onReadyRead()
{
    QDataStream stream(m_clientSocket);
    stream.setVersion(QDataStream::Qt_5_15);

    while (true) {
        if (static_cast<quint32>(m_buffer.size()) < sizeof(quint32)) {
            if (m_clientSocket->bytesAvailable() < static_cast<qint64>(sizeof(quint32))) {
                break;
            }
            m_buffer.append(m_clientSocket->read(static_cast<qint64>(sizeof(quint32)) - static_cast<qint64>(m_buffer.size())));
        }

        quint32 messageSize;
        QDataStream sizeStream(m_buffer);
        sizeStream >> messageSize;

        if (static_cast<quint32>(m_buffer.size()) < sizeof(quint32) + messageSize) {
            qint64 remaining = static_cast<qint64>(messageSize) - (static_cast<qint64>(m_buffer.size()) - static_cast<qint64>(sizeof(quint32)));
            if (m_clientSocket->bytesAvailable() < remaining) {
                break;
            }
            m_buffer.append(m_clientSocket->read(remaining));
        }

        QByteArray jsonData = m_buffer.mid(sizeof(quint32), messageSize);
        m_buffer.remove(0, sizeof(quint32) + messageSize);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj["type"].toString() == "login_success") {
                m_username = obj["username"].toString();
            }
            emit jsonReceived(obj);
        }
    }
}

void ChatClient::onConnected()
{
    emit connected();
}

void ChatClient::onDisconnected()
{
    m_username.clear();
    emit disconnected();
}

void ChatClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit error(m_clientSocket->errorString());
}
