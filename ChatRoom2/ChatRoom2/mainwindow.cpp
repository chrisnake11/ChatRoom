#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QString>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), _register_dialog(nullptr), _login_dialog(nullptr), _reset_dialog(nullptr), _chat_dialog(nullptr)
{
    // 初始化UI界面
    ui->setupUi(this);
    // 初始化登录注册界面
    _login_dialog = new LoginDialog(this);
    ui->stackedWidget->addWidget(_login_dialog);
    ui->stackedWidget->setCurrentWidget(_login_dialog);

    _login_dialog->show();

    // 切换到注册界面
    connect(_login_dialog, &LoginDialog::switch_to_register, this, &MainWindow::createRegisterDialog);

    // 切换到重置密码界面
    connect(_login_dialog, &LoginDialog::switch_to_reset, this, &MainWindow::createResetDialog);

    connect(_login_dialog, &LoginDialog::switch_to_chat, this, &MainWindow::createChatDialog);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createResetDialog()
{
    // 创建重置密码界面
    if (_reset_dialog == nullptr) {
        _reset_dialog = new ResetDialog(this);
        ui->stackedWidget->addWidget(_reset_dialog);

        // 重置密码界面返回登录界面
        connect(_reset_dialog, &ResetDialog::switch_to_login, this, [this] {
            ui->stackedWidget->setCurrentWidget(_login_dialog);
            });

        // 重置密码成功，显示提示信息并删除重置密码界面
        connect(_reset_dialog, &ResetDialog::reset_success, this, [this] {
            qDebug() << "重置密码成功";
            _login_dialog->showTip("重置密码成功，请登录", true);
            ui->stackedWidget->setCurrentWidget(_login_dialog);
            ui->stackedWidget->removeWidget(_reset_dialog);
            delete _reset_dialog;
            _reset_dialog = nullptr;
            });

    }
    ui->stackedWidget->setCurrentWidget(_reset_dialog);
}

void MainWindow::createRegisterDialog()
{
    // 如果为空，新建注册界面
    if (_register_dialog == nullptr) {
        _register_dialog = new RegisterDialog(this);
        ui->stackedWidget->addWidget(_register_dialog);

        // 注册界面返回登录界面
        connect(_register_dialog, &RegisterDialog::switch_to_login, this, [this] {
            ui->stackedWidget->setCurrentWidget(_login_dialog);
            });

        // 注册成功，显示提示信息并删除注册界面
        connect(_register_dialog, &RegisterDialog::register_success, this, [this] {
            qDebug() << "注册成功";
            _login_dialog->showTip("注册成功，请登录", true);
            // 先切换到登录界面，再删除注册界面
            ui->stackedWidget->setCurrentWidget(_login_dialog);
            ui->stackedWidget->removeWidget(_register_dialog);
            delete _register_dialog;
            _register_dialog = nullptr;
            });
    }

    // 直接切换到注册界面
    ui->stackedWidget->setCurrentWidget(_register_dialog);
}


void MainWindow::createChatDialog()
{
	qDebug() << "createChatDialog called";
	// 如果为空，新建聊天界面
    if (_chat_dialog == nullptr) {
        _chat_dialog = new ChatDialog(this);
        ui->stackedWidget->addWidget(_chat_dialog);
        
        // 注册聊天界面切换到其他界面的信号事件
	}

	ui->stackedWidget->setCurrentWidget(_chat_dialog);

}
