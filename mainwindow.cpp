#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "loginwindow.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QPoint>
#include <QSize>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_chatClient(new ChatClient(this))
    , m_database(new Database(this))
{
    ui->setupUi(this);
    setWindowTitle("即时通讯客户端");

    // 初始化数据库
    m_database->initializeDatabase("chat_client.db");

    // 连接信号
    connect(m_chatClient, &ChatClient::connected, this, [this]() {
        // 连接成功后，登录窗口会处理登录
    });
    connect(m_chatClient, &ChatClient::disconnected, this, &MainWindow::onDisconnected);
    connect(m_chatClient, &ChatClient::jsonReceived, this, &MainWindow::onJsonReceived);
    connect(m_chatClient, &ChatClient::error, this, [this](const QString &error) {
        QMessageBox::critical(this, "错误", error);
    });

    setupUI();

    // 显示登录窗口
    LoginWindow *loginWindow = new LoginWindow(this);
    bool loginSuccess = false;
    
    // 标记当前操作类型
    bool *isRegistering = new bool(false);
    
    connect(loginWindow, &LoginWindow::loginRequested, this, [this, loginWindow, isRegistering]() {
        *isRegistering = false;
        // 如果已经连接，直接发送登录消息
        if (m_chatClient->isConnected()) {
            QJsonObject loginMsg;
            loginMsg["type"] = "login";
            loginMsg["username"] = loginWindow->getUsername();
            loginMsg["password"] = loginWindow->getPassword();
            m_chatClient->sendJson(loginMsg);
        } else {
            m_chatClient->connectToServer(loginWindow->getServerAddress(), loginWindow->getServerPort());
        }
    });
    
    connect(loginWindow, &LoginWindow::registerRequested, this, [this, loginWindow, isRegistering]() {
        *isRegistering = true;
        // 如果已经连接，先断开
        if (m_chatClient->isConnected()) {
            m_chatClient->disconnectFromServer();
        }
        m_chatClient->connectToServer(loginWindow->getServerAddress(), loginWindow->getServerPort());
    });
    
    // 连接成功后发送登录/注册消息
    QMetaObject::Connection conn = connect(m_chatClient, &ChatClient::connected, this, [this, loginWindow, isRegistering]() {
        if (*isRegistering) {
            QJsonObject registerMsg;
            registerMsg["type"] = "register";
            registerMsg["username"] = loginWindow->getUsername();
            registerMsg["password"] = loginWindow->getPassword();
            registerMsg["nickname"] = loginWindow->findChild<QLineEdit*>("nicknameLineEdit")->text();
            m_chatClient->sendJson(registerMsg);
        } else {
            QJsonObject loginMsg;
            loginMsg["type"] = "login";
            loginMsg["username"] = loginWindow->getUsername();
            loginMsg["password"] = loginWindow->getPassword();
            m_chatClient->sendJson(loginMsg);
        }
    });
    
    // 登录成功后关闭登录窗口
    QMetaObject::Connection loginConn = connect(this, &MainWindow::loginSuccessful, loginWindow, [loginWindow, &loginSuccess]() {
        loginSuccess = true;
        loginWindow->accept();
    });
    
    loginWindow->exec();
    
    disconnect(conn);
    disconnect(loginConn);
    delete isRegistering;
    
    if (!loginSuccess) {
        close();
        return;
    }
}

MainWindow::~MainWindow()
{
    // 关闭所有聊天窗口
    for (auto it = m_chatWindows.begin(); it != m_chatWindows.end(); ++it) {
        it.value()->close();
    }
    delete ui;
}

void MainWindow::setupUI()
{
    // 初始化
    m_currentChatWindow = nullptr;
    m_currentChatTarget = "";
    
    // 设置列表样式
    ui->contactsListWidget->setAlternatingRowColors(true);
    ui->groupsListWidget->setAlternatingRowColors(true);
    
    // 设置 QStackedWidget 初始页面
    ui->stackedWidget->setCurrentIndex(0);  // 显示联系人页面
    
    // 初始隐藏返回按钮
    ui->backButton->setVisible(false);
    
    // 连接返回按钮
    connect(ui->backButton, &QPushButton::clicked, this, &MainWindow::on_backButton_clicked);
    
    // 设置主窗口样式
    setStyleSheet(
        "QMainWindow {"
        "background-color: #ffffff;"
        "}"
        "QListWidget {"
        "border: 1px solid #e0e0e0;"
        "border-radius: 4px;"
        "background-color: white;"
        "}"
        "QListWidget::item {"
        "padding: 8px;"
        "border-bottom: 1px solid #f0f0f0;"
        "}"
        "QListWidget::item:hover {"
        "background-color: #f5f5f5;"
        "}"
        "QListWidget::item:selected {"
        "background-color: #e3f2fd;"
        "color: #1976d2;"
        "}"
        "QPushButton {"
        "background-color: #0078d4;"
        "color: white;"
        "border: none;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "background-color: #005a9e;"
        "}"
        "QTabWidget::pane {"
        "border: 1px solid #e0e0e0;"
        "border-radius: 4px;"
        "background-color: white;"
        "}"
        "QTabBar::tab {"
        "background-color: #f5f5f5;"
        "padding: 8px 20px;"
        "border-top-left-radius: 4px;"
        "border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected {"
        "background-color: white;"
        "color: #0078d4;"
        "font-weight: bold;"
        "}"
    );
}

void MainWindow::onLoginSuccess()
{
    ui->statusLabel->setText(QString("已登录: %1").arg(m_username));
    ui->statusLabel->setStyleSheet("color: green;");
    emit loginSuccessful();
}

void MainWindow::onLoginFailed(const QString &message)
{
    QMessageBox::critical(this, "登录失败", message);
    close();
}

void MainWindow::onRegisterSuccess()
{
    QMessageBox::information(this, "成功", "注册成功！您现在可以使用该账号登录。");
    // 注册成功后不断开连接，用户可以直接登录
}

void MainWindow::onRegisterFailed(const QString &message)
{
    QMessageBox::critical(this, "注册失败", message);
}

void MainWindow::onDisconnected()
{
    QMessageBox::warning(this, "断开连接", "与服务器断开连接");
    close();
}

void MainWindow::onJsonReceived(const QJsonObject &docObj)
{
    QString type = docObj["type"].toString();

    if (type == "login_success") {
        m_username = docObj["username"].toString();
        onLoginSuccess();
    }
    else if (type == "login_failed") {
        onLoginFailed(docObj["message"].toString());
    }
    else if (type == "register_success") {
        onRegisterSuccess();
    }
    else if (type == "register_failed") {
        onRegisterFailed(docObj["message"].toString());
    }
    else if (type == "contacts_list") {
        onContactsListReceived(docObj["contacts"].toArray());
    }
    else if (type == "groups_list") {
        onGroupsListReceived(docObj["groups"].toArray());
    }
    else if (type == "private_message") {
        QString sender = docObj["sender"].toString();
        QString content = docObj["content"].toString();
        QString timestamp = docObj["timestamp"].toString();
        onPrivateMessageReceived(sender, content, timestamp);
    }
    else if (type == "group_message") {
        QString sender = docObj["sender"].toString();
        QString groupName = docObj["group_name"].toString();
        QString content = docObj["content"].toString();
        QString timestamp = docObj["timestamp"].toString();
        onGroupMessageReceived(sender, groupName, content, timestamp);
    }
    else if (type == "user_online") {
        onUserOnline(docObj["username"].toString());
    }
    else if (type == "user_offline") {
        onUserOffline(docObj["username"].toString());
    }
    else if (type == "offline_messages") {
        // 离线消息已经在loadHistory中加载了，不需要重复显示
        // 注释掉以避免重复显示
        // onOfflineMessagesReceived(docObj["messages"].toArray());
    }
    else if (type == "add_contact_success") {
        QMessageBox::information(this, "成功", "添加联系人成功");
        on_refreshButton_clicked();
    }
    else if (type == "create_group_success") {
        QMessageBox::information(this, "成功", "创建群组成功");
        on_refreshButton_clicked();
    }
    else if (type == "join_group_success") {
        QMessageBox::information(this, "成功", "加入群组成功");
        on_refreshButton_clicked();
    }
    else if (type == "add_group_members_result") {
        QString groupName = docObj["group_name"].toString();
        QJsonArray members = docObj["members"].toArray();
        if (members.isEmpty()) {
            QMessageBox::information(this, "提示", QString("没有新的成员被添加到群组 \"%1\"。").arg(groupName));
        } else {
            QStringList list;
            for (const QJsonValue &v : members) {
                list << v.toString();
            }
            QMessageBox::information(
                this,
                "添加群成员成功",
                QString("已将以下成员加入群组 \"%1\"：\n%2").arg(groupName, list.join(", "))
            );
        }
        // 群组列表会通过服务器下发的 groups_list 更新
    }
    else if (type == "added_to_group") {
        QString groupName = docObj["group_name"].toString();
        QString inviter = docObj["inviter"].toString();
        QMessageBox::information(
            this,
            "群聊邀请",
            QString("你已被 %1 邀请加入群组 \"%2\"。").arg(inviter, groupName)
        );
        // 具体的群组列表刷新由服务器额外发送的 groups_list 处理
    }
}

void MainWindow::onContactsListReceived(const QJsonArray &contacts)
{
    ui->contactsListWidget->clear();
    for (const QJsonValue &value : contacts) {
        QJsonObject contact = value.toObject();
        QString username = contact["username"].toString();
        QString nickname = contact["nickname"].toString();
        bool online = contact["status"].toInt() == 1;

        QString displayText = QString("%1 (%2)").arg(nickname, username);
        if (online) {
            displayText += " [在线]";
        }

        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, username);
        ui->contactsListWidget->addItem(item);
    }
}

void MainWindow::onGroupsListReceived(const QJsonArray &groups)
{
    ui->groupsListWidget->clear();
    for (const QJsonValue &value : groups) {
        QJsonObject group = value.toObject();
        QString groupName = group["group_name"].toString();

        QListWidgetItem *item = new QListWidgetItem(groupName);
        item->setData(Qt::UserRole, groupName);
        ui->groupsListWidget->addItem(item);
    }
}

void MainWindow::onPrivateMessageReceived(const QString &sender, const QString &content, const QString &timestamp)
{
    // 如果sender是自己，说明这是自己发送的消息确认，不需要重复显示
    if (!sender.isEmpty() && sender == m_username) {
        // 自己发送的消息已经在发送时显示了，不需要重复
        return;
    }
    
    // 不再保存到本地数据库，避免重复显示
    // m_database->saveMessage(sender, m_username, content, "private", "", timestamp);

    // 打开或更新聊天窗口（target是sender，因为这是对方发送的消息）
    openChatWindow(sender, "private");
    if (m_chatWindows.contains(sender)) {
        // 添加对方发送的消息（sender是对方，不是自己）
        m_chatWindows[sender]->addMessage(sender, content, timestamp);
    }
}

void MainWindow::onGroupMessageReceived(const QString &sender, const QString &groupName, const QString &content, const QString &timestamp)
{
    // 不再保存到本地数据库，避免重复显示
    // m_database->saveMessage(sender, m_username, content, "group", groupName, timestamp);

    // 打开或更新聊天窗口
    openChatWindow(groupName, "group");
    if (m_chatWindows.contains(groupName)) {
        m_chatWindows[groupName]->addMessage(sender, content, timestamp);
    }
}

void MainWindow::onUserOnline(const QString &username)
{
    updateContactStatus(username, true);
}

void MainWindow::onUserOffline(const QString &username)
{
    updateContactStatus(username, false);
}

void MainWindow::onOfflineMessagesReceived(const QJsonArray &messages)
{
    // 离线消息只显示一次，不保存到本地数据库，避免重复显示
    for (const QJsonValue &value : messages) {
        QJsonObject msg = value.toObject();
        QString type = msg["message_type"].toString();
        QString sender = msg["sender"].toString();
        QString content = msg["content"].toString();
        QString timestamp = msg["timestamp"].toString();

        if (type == "private") {
            // 不保存到数据库，只显示
            openChatWindow(sender, "private");
            if (m_chatWindows.contains(sender)) {
                m_chatWindows[sender]->addMessage(sender, content, timestamp);
            }
        } else {
            QString groupName = msg["group_name"].toString();
            // 不保存到数据库，只显示
            openChatWindow(groupName, "group");
            if (m_chatWindows.contains(groupName)) {
                m_chatWindows[groupName]->addMessage(sender, content, timestamp);
            }
        }
    }
}

void MainWindow::on_addContactButton_clicked()
{
    bool ok;
    QString contactUsername = QInputDialog::getText(this, "添加联系人", "请输入用户名:", QLineEdit::Normal, "", &ok);
    if (ok && !contactUsername.isEmpty()) {
        QJsonObject msg;
        msg["type"] = "add_contact";
        msg["contact_username"] = contactUsername;
        m_chatClient->sendJson(msg);
    }
}

void MainWindow::on_createGroupButton_clicked()
{
    bool ok;
    QString groupName = QInputDialog::getText(this, "创建群组", "请输入群组名称:", QLineEdit::Normal, "", &ok);
    if (ok && !groupName.isEmpty()) {
        QJsonObject msg;
        msg["type"] = "create_group";
        msg["group_name"] = groupName;
        m_chatClient->sendJson(msg);
    }
}

void MainWindow::on_addGroupMemberButton_clicked()
{
    // 先确认选中了哪个群
    QListWidgetItem *groupItem = ui->groupsListWidget->currentItem();
    if (!groupItem) {
        QMessageBox::warning(this, "提示", "请先在群组列表中选择一个群聊。");
        return;
    }

    QString groupName = groupItem->data(Qt::UserRole).toString();
    if (groupName.isEmpty()) {
        groupName = groupItem->text();
    }

    // 从联系人列表中收集所有联系人用户名
    QStringList contactNames;
    for (int i = 0; i < ui->contactsListWidget->count(); ++i) {
        QListWidgetItem *item = ui->contactsListWidget->item(i);
        QString username = item->data(Qt::UserRole).toString();
        if (username.isEmpty())
            username = item->text();
        if (!username.isEmpty())
            contactNames << username;
    }

    if (contactNames.isEmpty()) {
        QMessageBox::information(this, "提示", "当前还没有联系人，无法添加群成员。");
        return;
    }

    // 使用自定义对话框支持多选联系人
    QDialog dialog(this);
    dialog.setWindowTitle("添加群成员");
    dialog.resize(300, 400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel("请选择要添加到群聊的联系人（可多选）：", &dialog);
    layout->addWidget(label);

    QListWidget *listWidget = new QListWidget(&dialog);
    listWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    // 填充联系人列表
    for (const QString &name : contactNames) {
        QListWidgetItem *item = new QListWidgetItem(name, listWidget);
        item->setData(Qt::UserRole, name);
    }
    layout->addWidget(listWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal, &dialog);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 收集所有选中的联系人
    QList<QListWidgetItem*> selectedItems = listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "未选择任何联系人。");
        return;
    }

    // 构造添加群成员请求，可以一次拉多个人
    QJsonObject msg;
    msg["type"] = "add_group_members";
    msg["group_name"] = groupName;
    QJsonArray members;
    for (QListWidgetItem *item : selectedItems) {
        QString name = item->data(Qt::UserRole).toString();
        if (!name.isEmpty())
            members.append(name);
    }
    msg["members"] = members;

    m_chatClient->sendJson(msg);
}
void MainWindow::on_contactsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString username = item->data(Qt::UserRole).toString();
    openChatWindow(username, "private");
}

void MainWindow::on_groupsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString groupName = item->data(Qt::UserRole).toString();
    openChatWindow(groupName, "group");
}

void MainWindow::on_logoutButton_clicked()
{
    m_chatClient->disconnectFromServer();
    close();
}

void MainWindow::on_refreshButton_clicked()
{
    // 刷新联系人列表和群组列表（服务器会在登录时自动发送）
    // 这里可以发送请求消息
}

void MainWindow::chatWindowClosed(const QString &target)
{
    if (m_chatWindows.contains(target)) {
        ChatWindow *window = m_chatWindows[target];
        m_chatWindows.remove(target);
        if (m_currentChatWindow == window) {
            m_currentChatWindow = nullptr;
            m_currentChatTarget = "";
            showContactsPage();
        }
        window->deleteLater();
    }
}

void MainWindow::on_backButton_clicked()
{
    showContactsPage();
}

void MainWindow::openChatWindow(const QString &target, const QString &type)
{
    // 如果已经打开过这个聊天窗口，直接切换到它
    if (m_chatWindows.contains(target)) {
        m_currentChatWindow = m_chatWindows[target];
        m_currentChatTarget = target;
        showChatPage();
        return;
    }
    
    // 创建新的聊天窗口
    ChatWindow *chatWindow = new ChatWindow(target, type, m_chatClient, m_database, m_username, ui->chatContainer);
    
    // 设置聊天窗口的布局
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->chatContainer->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui->chatContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->chatContainer->setLayout(layout);
    }
    
    // 移除旧的聊天窗口（如果有）
    if (m_currentChatWindow) {
        layout->removeWidget(m_currentChatWindow);
        m_currentChatWindow->hide();
    }
    
    // 添加新的聊天窗口
    layout->addWidget(chatWindow);
    chatWindow->show();
    
    // 连接信号
    connect(chatWindow, &ChatWindow::backButtonClicked, this, &MainWindow::on_backButton_clicked);
    
    // 保存引用
    m_chatWindows[target] = chatWindow;
    m_currentChatWindow = chatWindow;
    m_currentChatTarget = target;
    
    // 切换到聊天页面
    showChatPage();
}

void MainWindow::showContactsPage()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->backButton->setVisible(false);
}

void MainWindow::showChatPage()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->backButton->setVisible(true);
    if (m_currentChatWindow) {
        m_currentChatWindow->show();
    }
}

void MainWindow::updateContactStatus(const QString &username, bool online)
{
    for (int i = 0; i < ui->contactsListWidget->count(); ++i) {
        QListWidgetItem *item = ui->contactsListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == username) {
            QString text = item->text();
            if (online && !text.contains("[在线]")) {
                text += " [在线]";
                item->setText(text);
            } else if (!online) {
                text.remove(" [在线]");
                item->setText(text);
            }
            break;
        }
    }
}
