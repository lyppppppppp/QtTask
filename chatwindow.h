#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QString>
#include "chatclient.h"
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ChatWindow; }
QT_END_NAMESPACE

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(const QString &target, const QString &type,
                       ChatClient *client, Database *db, const QString &currentUser,
                       QWidget *parent = nullptr);
    ~ChatWindow();

    void addMessage(const QString &sender, const QString &content, const QString &timestamp);
    void loadHistory();

signals:
    void windowClosed(const QString &target);
    void backButtonClicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_sendButton_clicked();
    void on_messageLineEdit_returnPressed();
    void on_backButton_clicked();

private:
    Ui::ChatWindow *ui;
    QString m_target;
    QString m_type;
    QString m_currentUser;
    ChatClient *m_chatClient;
    Database *m_database;
    QDateTime m_lastMessageTime;  // 上一条消息的时间，用于时间分组
    bool m_isFirstMessage;  // 是否是第一条消息

    void setupUI();
    void displayMessage(const QString &sender, const QString &content, const QString &timestamp, bool isSelf);
    void displayTimeDivider(const QString &timeStr);
    bool shouldShowTimeDivider(const QDateTime &currentTime);
};

#endif // CHATWINDOW_H
