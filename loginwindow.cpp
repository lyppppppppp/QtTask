#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QMessageBox>
#include <QHostAddress>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    setWindowTitle("登录");
    setFixedSize(400, 350);
    setupUI();
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::setupUI()
{
    ui->serverAddressLineEdit->setText("127.0.0.1");
    ui->serverPortSpinBox->setValue(8888);
    ui->serverPortSpinBox->setMinimum(1024);
    ui->serverPortSpinBox->setMaximum(65535);
    ui->registerModeCheckBox->setChecked(false);
    ui->registerWidget->setVisible(false);
}

QString LoginWindow::getUsername() const
{
    return ui->usernameLineEdit->text().trimmed();
}

QString LoginWindow::getPassword() const
{
    return ui->passwordLineEdit->text();
}

QHostAddress LoginWindow::getServerAddress() const
{
    return QHostAddress(ui->serverAddressLineEdit->text().trimmed());
}

quint16 LoginWindow::getServerPort() const
{
    return static_cast<quint16>(ui->serverPortSpinBox->value());
}

bool LoginWindow::isRegisterMode() const
{
    return ui->registerModeCheckBox->isChecked();
}

void LoginWindow::on_loginButton_clicked()
{
    if (getUsername().isEmpty() || getPassword().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入用户名和密码！");
        return;
    }

    if (getServerAddress().isNull()) {
        QMessageBox::warning(this, "警告", "请输入有效的服务器地址！");
        return;
    }

    emit loginRequested();
}

void LoginWindow::on_registerButton_clicked()
{
    if (getUsername().isEmpty() || getPassword().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入用户名和密码！");
        return;
    }

    if (ui->nicknameLineEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入昵称！");
        return;
    }

    if (getServerAddress().isNull()) {
        QMessageBox::warning(this, "警告", "请输入有效的服务器地址！");
        return;
    }

    emit registerRequested();
}

void LoginWindow::on_registerModeCheckBox_toggled(bool checked)
{
    ui->registerWidget->setVisible(checked);
    if (checked) {
        ui->loginButton->setText("注册");
    } else {
        ui->loginButton->setText("登录");
    }
}
