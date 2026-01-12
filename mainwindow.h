#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QListWidgetItem>
#include <QStackedWidget>
#include "chatclient.h"
#include "database.h"
#include "chatwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void loginSuccessful();

private slots:
    void onLoginSuccess();
    void onLoginFailed(const QString &message);
    void onRegisterSuccess();
    void onRegisterFailed(const QString &message);
    void onDisconnected();
    void onJsonReceived(const QJsonObject &docObj);
    void onContactsListReceived(const QJsonArray &contacts);
    void onGroupsListReceived(const QJsonArray &groups);
    void onPrivateMessageReceived(const QString &sender, const QString &content, const QString &timestamp);
    void onGroupMessageReceived(const QString &sender, const QString &groupName, const QString &content, const QString &timestamp);
    void onUserOnline(const QString &username);
    void onUserOffline(const QString &username);
    // 处理离线消息（当前实现中只是显示，不再区分是否保存到数据库）
    void onOfflineMessagesReceived(const QJsonArray &messages);

    void on_addContactButton_clicked();
    void on_createGroupButton_clicked();
    void on_addGroupMemberButton_clicked();
    void on_contactsListWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_groupsListWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_logoutButton_clicked();
    void on_refreshButton_clicked();

    void chatWindowClosed(const QString &target);
    void on_backButton_clicked();

private:
    Ui::MainWindow *ui;
    ChatClient *m_chatClient;
    Database *m_database;
    QString m_username;
    QMap<QString, ChatWindow*> m_chatWindows;  // target -> window
    ChatWindow *m_currentChatWindow;  // 当前显示的聊天窗口
    QString m_currentChatTarget;  // 当前聊天目标

    void setupUI();
    void openChatWindow(const QString &target, const QString &type);
    void showContactsPage();
    void showChatPage();
    void updateContactStatus(const QString &username, bool online);
};

#endif // MAINWINDOW_H
