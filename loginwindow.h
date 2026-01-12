#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    QString getUsername() const;
    QString getPassword() const;
    QHostAddress getServerAddress() const;
    quint16 getServerPort() const;
    bool isRegisterMode() const;

signals:
    void loginRequested();
    void registerRequested();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();
    void on_registerModeCheckBox_toggled(bool checked);

private:
    Ui::LoginWindow *ui;
    void setupUI();
};

#endif // LOGINWINDOW_H
