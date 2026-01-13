#include "chatwindow.h"
#include "ui_chatwindow.h"
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollBar>
#include <QCloseEvent>
#include <QMessageBox>

ChatWindow::ChatWindow(const QString &target, const QString &type,
                       ChatClient *client, Database *db, const QString &currentUser,
                       QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
    , m_target(target)
    , m_type(type)
    , m_currentUser(currentUser)
    , m_chatClient(client)
    , m_database(db)
    , m_isFirstMessage(true)
{
    ui->setupUi(this);
    setupUI();
    loadHistory();
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::setupUI()
{
    // 设置消息显示区域样式（类似微信的聊天背景）
    ui->messageTextEdit->setReadOnly(true);
    ui->messageTextEdit->setFont(QFont("Microsoft YaHei", 10));
    ui->messageTextEdit->setStyleSheet(
        "QTextEdit {"
        "background-color: #ededed;"
        "border: none;"
        "padding: 0px;"
        "}"
    );
    // 确保聊天内容整体左上对齐显示
    ui->messageTextEdit->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    
    // 设置输入框样式
    ui->messageLineEdit->setStyleSheet(
        "QLineEdit {"
        "padding: 8px;"
        "border: 1px solid #ddd;"
        "border-radius: 4px;"
        "font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "border: 1px solid #0078d4;"
        "}"
    );
    
    // 设置发送按钮样式
    ui->sendButton->setStyleSheet(
        "QPushButton {"
        "background-color: #0078d4;"
        "color: white;"
        "border: none;"
        "padding: 8px 20px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "background-color: #005a9e;"
        "}"
    );
}

void ChatWindow::addMessage(const QString &sender, const QString &content, const QString &timestamp)
{
    // 确保正确判断消息是否是自己发送的
    // 只有当sender完全等于m_currentUser时，才是自己发送的消息
    bool isSelf = (!sender.isEmpty() && sender == m_currentUser);
    
    // 调试输出（可以删除）
    // qDebug() << "addMessage - sender:" << sender << "currentUser:" << m_currentUser << "isSelf:" << isSelf;
    
    displayMessage(sender, content, timestamp, isSelf);
}

void ChatWindow::displayTimeDivider(const QString &timeStr)
{
    QString dividerText = QString("<div style='text-align: center; margin: 15px 0; padding: 0 10px;'>"
                                 "<div style='display: inline-block; background-color: rgba(0,0,0,0.05); "
                                 "padding: 4px 12px; border-radius: 12px; font-size: 11px; color: #999;'>"
                                 "%1"
                                 "</div>"
                                 "</div>")
                          .arg(timeStr);
    ui->messageTextEdit->append(dividerText);
}

bool ChatWindow::shouldShowTimeDivider(const QDateTime &currentTime)
{
    if (m_isFirstMessage) {
        // 第一条消息显示时间分组
        m_isFirstMessage = false;
        m_lastMessageTime = currentTime;
        return true;
    }
    
    if (!m_lastMessageTime.isValid()) {
        m_lastMessageTime = currentTime;
        return true;
    }
    
    // 计算时间差（秒），使用绝对值确保为正数
    qint64 secondsDiff = qAbs(m_lastMessageTime.secsTo(currentTime));
    
    // 如果间隔>=5分钟（300秒），显示时间分组
    if (secondsDiff >= 300) {
        m_lastMessageTime = currentTime;
        return true;
    }
    
    // 即使不显示时间分组，也要更新最后一条消息的时间
    m_lastMessageTime = currentTime;
    return false;
}

void ChatWindow::displayMessage(const QString &sender, const QString &content, const QString &timestamp, bool isSelf)
{
    QDateTime msgTime;
    QString timeStr = timestamp;
    
    // 解析时间
    if (timeStr.isEmpty()) {
        msgTime = QDateTime::currentDateTime();
        timeStr = msgTime.toString("hh:mm");
    } else {
        msgTime = QDateTime::fromString(timeStr, Qt::ISODate);
        if (msgTime.isValid()) {
            timeStr = msgTime.toString("hh:mm");
        } else {
            msgTime = QDateTime::currentDateTime();
            timeStr = msgTime.toString("hh:mm");
        }
    }
    
    // 检查是否需要显示时间分组（在调用前保存状态）
    bool wasFirstMessage = m_isFirstMessage;
    bool needTimeDivider = shouldShowTimeDivider(msgTime);
    
    if (needTimeDivider) {
        // 格式化完整时间显示（用于时间分组）
        QString fullTimeStr;
        if (wasFirstMessage) {
            fullTimeStr = "聊天开始 " + msgTime.toString("hh:mm");
        } else {
            // 判断是否需要显示完整日期
            QDate lastDate = m_lastMessageTime.date();
            QDate currentDate = msgTime.date();
            if (lastDate != currentDate) {
                fullTimeStr = msgTime.toString("yyyy年MM月dd日 hh:mm");
            } else {
                fullTimeStr = msgTime.toString("hh:mm");
            }
        }
        displayTimeDivider(fullTimeStr);
    }
    
    QString displayText;
    if (m_type == "private") {
        if (isSelf) {
            // 自己发送的消息 - 显示在左边，绿色背景，不显示名字，不显示时间
            displayText = QString("<div style='margin: 8px 0; padding: 0 10px;'>"
                                 "<div style='text-align: left; clear: both;'>"
                                 "<div style='display: inline-block; max-width: 75%%; position: relative;'>"
                                 "<div style='background-color: #95ec69; color: #000; padding: 8px 12px; border-radius: 6px; word-wrap: break-word; "
                                 "box-shadow: 0 1px 2px rgba(0,0,0,0.1); line-height: 1.4; font-size: 14px;'>"
                                 "%1"
                                 "</div>"
                                 "</div>"
                                 "</div>"
                                 "</div>")
                          .arg(content.toHtmlEscaped().replace("\n", "<br/>"));
        } else {
            // 对方发送的消息 - 显示在左边，白色背景，显示发送者名称，不显示时间
            displayText = QString("<div style='margin: 8px 0; padding: 0 10px;'>"
                                 "<div style='text-align: left; clear: both;'>"
                                 "<div style='display: inline-block; max-width: 75%%; position: relative;'>"
                                 "<div style='font-weight: 500; color: #576b95; margin-bottom: 4px; font-size: 12px; padding-left: 2px;'>"
                                 "%1"
                                 "</div>"
                                 "<div style='background-color: #ffffff; color: #000; padding: 8px 12px; border-radius: 6px; word-wrap: break-word; "
                                 "box-shadow: 0 1px 2px rgba(0,0,0,0.1); line-height: 1.4; font-size: 14px; border: 1px solid #e5e5e5;'>"
                                 "%2"
                                 "</div>"
                                 "</div>"
                                 "</div>"
                                 "</div>")
                          .arg(sender.toHtmlEscaped(), content.toHtmlEscaped().replace("\n", "<br/>"));
        }
    } else {
        // 群组消息 - 所有消息都显示在左边，显示发送者名称，不显示时间
        displayText = QString("<div style='margin: 8px 0; padding: 0 10px;'>"
                             "<div style='text-align: left; clear: both;'>"
                             "<div style='display: inline-block; max-width: 75%%;'>"
                             "<div style='font-weight: 500; color: #576b95; margin-bottom: 4px; font-size: 12px;'>"
                             "%1"
                             "</div>"
                             "<div style='background-color: #ffffff; color: #000; padding: 8px 12px; border-radius: 6px; word-wrap: break-word; "
                             "box-shadow: 0 1px 2px rgba(0,0,0,0.1); line-height: 1.4; font-size: 14px; border: 1px solid #e5e5e5;'>"
                             "%2"
                             "</div>"
                             "</div>"
                             "</div>"
                             "</div>")
                      .arg(sender.toHtmlEscaped(), content.toHtmlEscaped().replace("\n", "<br/>"));
    }

    ui->messageTextEdit->append(displayText);

    // 滚动到底部
    QScrollBar *scrollBar = ui->messageTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatWindow::loadHistory()
{
    // 先清空显示区域，避免重复显示
    ui->messageTextEdit->clear();
    
    // 重置时间分组状态
    m_isFirstMessage = true;
    m_lastMessageTime = QDateTime();
    
    // 不再从数据库加载历史消息，避免每次登录重复显示
    // 只显示本次会话的消息
    // QJsonArray messages = m_database->getMessages(m_target, m_type, m_currentUser, 50);
    // 
    // // 从旧到新显示
    // for (int i = messages.size() - 1; i >= 0; --i) {
    //     QJsonObject msg = messages[i].toObject();
    //     QString sender = msg["sender"].toString();
    //     QString content = msg["content"].toString();
    //     QString timestamp = msg["timestamp"].toString();
    //     
    //     // 确保正确判断：只有当sender完全等于m_currentUser时，才是自己发送的
    //     bool isSelf = (!sender.isEmpty() && sender == m_currentUser);
    //     
    //     displayMessage(sender, content, timestamp, isSelf);
    // }
    
    // 滚动到底部
    QScrollBar *scrollBar = ui->messageTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatWindow::on_sendButton_clicked()
{
    QString content = ui->messageLineEdit->text().trimmed();
    if (content.isEmpty()) {
        return;
    }

    // 立即显示自己发送的消息（优化体验，类似微信）
    // 确保使用正确的sender（m_currentUser）和isSelf=true
    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 直接调用displayMessage，确保isSelf=true
    displayMessage(m_currentUser, content, currentTime, true);

    QJsonObject message;
    if (m_type == "private") {
        message["type"] = "private_message";
        message["receiver"] = m_target;
    } else {
        message["type"] = "group_message";
        message["group_name"] = m_target;
    }
    message["content"] = content;

    m_chatClient->sendJson(message);

    // 不再保存到本地数据库，避免重复显示
    // m_database->saveMessage(m_currentUser, m_target, content, m_type, 
    //                        m_type == "group" ? m_target : "", currentTime);

    // 清空输入框
    ui->messageLineEdit->clear();
}

void ChatWindow::on_messageLineEdit_returnPressed()
{
    on_sendButton_clicked();
}

void ChatWindow::on_backButton_clicked()
{
    emit backButtonClicked();
}

void ChatWindow::closeEvent(QCloseEvent *event)
{
    emit windowClosed(m_target);
    event->accept();
}
