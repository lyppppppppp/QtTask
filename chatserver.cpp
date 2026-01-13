#include "chatserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ChatServer::ChatServer(Database *db, QObject *parent)
    : QTcpServer(parent)
    , m_database(db)
{
}

ChatServer::~ChatServer()
{
    stopServer();
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    ServerWorker *worker = new ServerWorker(this);
    if (!worker->setSocketDescriptor(socketDescriptor)) {
        worker->deleteLater();
        return;
    }

    connect(worker, &ServerWorker::jsonReceived, this, &ChatServer::jsonReceived);
    connect(worker, &ServerWorker::disconnectedFromClient, this, [this, worker]() {
        onUserDisconnected(worker);
    });
    connect(worker, &ServerWorker::error, this, [this](QAbstractSocket::SocketError socketError) {
        emit logMessage(QString("Socket错误: %1").arg(socketError));
    });

    emit logMessage(QString("新客户端连接: %1").arg(socketDescriptor));
}

void ChatServer::stopServer()
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it.value()->disconnectFromClient();
    }
    m_clients.clear();
    close();
}

void ChatServer::jsonReceived(ServerWorker *sender, const QJsonObject &docObj)
{
    QString type = docObj["type"].toString();

    if (type == "login") {
        QString username = docObj["username"].toString();
        QString password = docObj["password"].toString();

        if (m_database->authenticateUser(username, password)) {
            sender->setUsername(username);
            m_clients[username] = sender;

            m_database->updateUserStatus(username, true);

            QJsonObject response;
            response["type"] = "login_success";
            response["username"] = username;
            response["userInfo"] = m_database->getUserInfo(username);
            sender->sendJson(response);

            // 发送联系人列表
            QJsonObject contactsMsg;
            contactsMsg["type"] = "contacts_list";
            contactsMsg["contacts"] = m_database->getContacts(username);
            sender->sendJson(contactsMsg);

            // 发送群组列表
            QJsonObject groupsMsg;
            groupsMsg["type"] = "groups_list";
            groupsMsg["groups"] = m_database->getUserGroups(username);
            sender->sendJson(groupsMsg);

            // 发送离线消息
            QJsonArray offlineMessages = m_database->getOfflineMessages(username);
            if (offlineMessages.size() > 0) {
                QJsonObject offlineMsg;
                offlineMsg["type"] = "offline_messages";
                offlineMsg["messages"] = offlineMessages;
                sender->sendJson(offlineMsg);
                
                // 标记这些离线消息为已读，避免下次登录重复显示
                for (const QJsonValue &value : offlineMessages) {
                    QJsonObject msg = value.toObject();
                    qint64 messageId = msg["id"].toVariant().toLongLong();
                    m_database->markMessageAsRead(messageId);
                }
            }

            emit logMessage(QString("用户登录: %1").arg(username));
            emit userConnected(username);

            // 通知其他用户
            QJsonObject notifyMsg;
            notifyMsg["type"] = "user_online";
            notifyMsg["username"] = username;
            broadcastToAll(notifyMsg, sender);
        } else {
            QJsonObject response;
            response["type"] = "login_failed";
            response["message"] = "用户名或密码错误";
            sender->sendJson(response);
            emit logMessage(QString("登录失败: %1").arg(username));
        }
    }
    else if (type == "register") {
        QString username = docObj["username"].toString();
        QString password = docObj["password"].toString();
        QString nickname = docObj["nickname"].toString();

        if (m_database->createUser(username, password, nickname)) {
            QJsonObject response;
            response["type"] = "register_success";
            response["message"] = "注册成功";
            sender->sendJson(response);
            emit logMessage(QString("新用户注册: %1").arg(username));
        } else {
            QJsonObject response;
            response["type"] = "register_failed";
            response["message"] = "用户名已存在";
            sender->sendJson(response);
        }
    }
    else if (type == "private_message") {
        QString receiver = docObj["receiver"].toString();
        QString senderUsername = sender->getUsername();
        QString content = docObj["content"].toString();

        // 保存消息到数据库
        m_database->saveMessage(senderUsername, receiver, content, "private");

        QJsonObject message;
        message["type"] = "private_message";
        message["sender"] = senderUsername;
        message["receiver"] = receiver;
        message["content"] = content;
        message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        // 如果接收者在线，直接发送；否则标记为离线消息
        if (m_clients.contains(receiver)) {
            sendToUser(receiver, message);
        }

        // 也发送给发送者（确认）
        sender->sendJson(message);
    }
    else if (type == "group_message") {
        QString groupName = docObj["group_name"].toString();
        QString senderUsername = sender->getUsername();
        QString content = docObj["content"].toString();

        // 保存消息到数据库
        m_database->saveMessage(senderUsername, "", content, "group", groupName);

        QJsonObject message;
        message["type"] = "group_message";
        message["sender"] = senderUsername;
        message["group_name"] = groupName;
        message["content"] = content;
        message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        sendToGroup(groupName, message, sender);
    }
    else if (type == "add_contact") {
        QString contactUsername = docObj["contact_username"].toString();
        QString username = sender->getUsername();

        if (m_database->addContact(username, contactUsername)) {
            QJsonObject response;
            response["type"] = "add_contact_success";
            response["contact"] = m_database->getUserInfo(contactUsername);
            sender->sendJson(response);

            // 更新联系人列表
            QJsonObject contactsMsg;
            contactsMsg["type"] = "contacts_list";
            contactsMsg["contacts"] = m_database->getContacts(username);
            sender->sendJson(contactsMsg);
        } else {
            QJsonObject response;
            response["type"] = "add_contact_failed";
            response["message"] = "添加联系人失败";
            sender->sendJson(response);
        }
    }
    else if (type == "create_group") {
        QString groupName = docObj["group_name"].toString();
        QString creator = sender->getUsername();

        if (m_database->createGroup(groupName, creator)) {
            QJsonObject response;
            response["type"] = "create_group_success";
            response["group_name"] = groupName;
            sender->sendJson(response);

            // 更新群组列表
            QJsonObject groupsMsg;
            groupsMsg["type"] = "groups_list";
            groupsMsg["groups"] = m_database->getUserGroups(creator);
            sender->sendJson(groupsMsg);
        } else {
            QJsonObject response;
            response["type"] = "create_group_failed";
            response["message"] = "创建群组失败";
            sender->sendJson(response);
        }
    }
    else if (type == "join_group") {
        QString groupName = docObj["group_name"].toString();
        QString username = sender->getUsername();

        if (m_database->addUserToGroup(groupName, username)) {
            QJsonObject response;
            response["type"] = "join_group_success";
            response["group_name"] = groupName;
            sender->sendJson(response);

            // 更新群组列表
            QJsonObject groupsMsg;
            groupsMsg["type"] = "groups_list";
            groupsMsg["groups"] = m_database->getUserGroups(username);
            sender->sendJson(groupsMsg);
        } else {
            QJsonObject response;
            response["type"] = "join_group_failed";
            response["message"] = "加入群组失败";
            sender->sendJson(response);
        }
    }
    else if (type == "add_group_members") {
        QString groupName = docObj["group_name"].toString();
        QString inviter = sender->getUsername();
        QJsonArray members = docObj["members"].toArray();

        QJsonArray addedMembers;

        for (const QJsonValue &val : members) {
            QString memberUsername = val.toString();
            if (memberUsername.isEmpty())
                continue;

            // 必须是邀请人的联系人，且不重复加入
            if (!m_database->isContact(inviter, memberUsername))
                continue;
            if (m_database->isGroupMember(groupName, memberUsername))
                continue;

            if (m_database->addUserToGroup(groupName, memberUsername)) {
                addedMembers.append(memberUsername);

                // 如果该用户在线，通知其被拉入群聊，并更新其群组列表
                if (m_clients.contains(memberUsername)) {
                    ServerWorker *worker = m_clients.value(memberUsername);

                    QJsonObject notify;
                    notify["type"] = "added_to_group";
                    notify["group_name"] = groupName;
                    notify["inviter"] = inviter;
                    worker->sendJson(notify);

                    QJsonObject groupsMsg;
                    groupsMsg["type"] = "groups_list";
                    groupsMsg["groups"] = m_database->getUserGroups(memberUsername);
                    worker->sendJson(groupsMsg);
                }
            }
        }

        // 给邀请人返回结果，并刷新其群组列表
        QJsonObject response;
        response["type"] = "add_group_members_result";
        response["group_name"] = groupName;
        response["members"] = addedMembers;
        sender->sendJson(response);

        QJsonObject groupsMsg;
        groupsMsg["type"] = "groups_list";
        groupsMsg["groups"] = m_database->getUserGroups(inviter);
        sender->sendJson(groupsMsg);
    }
    else if (type == "get_history") {
        QString target = docObj["target"].toString();
        QString messageType = docObj["message_type"].toString();
        QString username = sender->getUsername();

        QJsonArray messages = m_database->getMessages(username, target, messageType);

        QJsonObject response;
        response["type"] = "history_messages";
        response["target"] = target;
        response["message_type"] = messageType;
        response["messages"] = messages;
        sender->sendJson(response);
    }
}

void ChatServer::onUserDisconnected(ServerWorker *sender)
{
    QString username = sender->getUsername();
    if (!username.isEmpty()) {
        m_clients.remove(username);
        m_database->updateUserStatus(username, false);

        QJsonObject notifyMsg;
        notifyMsg["type"] = "user_offline";
        notifyMsg["username"] = username;
        broadcastToAll(notifyMsg, sender);

        emit logMessage(QString("用户断开连接: %1").arg(username));
        emit userDisconnected(username);
    }

    sender->deleteLater();
}

void ChatServer::broadcastToAll(const QJsonObject &message, ServerWorker *exclude)
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.value() != exclude) {
            it.value()->sendJson(message);
        }
    }
}

void ChatServer::sendToUser(const QString &username, const QJsonObject &message)
{
    if (m_clients.contains(username)) {
        m_clients[username]->sendJson(message);
    }
}

void ChatServer::sendToGroup(const QString &groupName, const QJsonObject &message, ServerWorker *exclude)
{
    QJsonArray members = m_database->getGroupMembers(groupName);
    for (const QJsonValue &value : members) {
        QJsonObject member = value.toObject();
        QString username = member["username"].toString();
        if (m_clients.contains(username)) {
            ServerWorker *worker = m_clients[username];
            if (worker != exclude) {
                worker->sendJson(message);
            }
        }
    }
}
