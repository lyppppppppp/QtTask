#include "serverworker.h"
#include <QDebug>

ServerWorker::ServerWorker(QObject *parent)
    : QObject(parent)
    , m_clientSocket(new QTcpSocket(this))
{
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &ServerWorker::receiveJson);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &ServerWorker::disconnectedFromClient);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_clientSocket, &QAbstractSocket::errorOccurred, this, &ServerWorker::error);
#else
    connect(m_clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &ServerWorker::error);
#endif
}

ServerWorker::~ServerWorker()
{
    if (m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        m_clientSocket->disconnectFromHost();
    }
}

bool ServerWorker::setSocketDescriptor(qintptr socketDescriptor)
{
    return m_clientSocket->setSocketDescriptor(socketDescriptor);
}

void ServerWorker::disconnectFromClient()
{
    m_clientSocket->disconnectFromHost();
}

void ServerWorker::sendJson(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream << static_cast<quint32>(data.size());
    packet.append(data);
    m_clientSocket->write(packet);
}

void ServerWorker::receiveJson()
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
            emit jsonReceived(this, doc.object());
        }
    }
}
