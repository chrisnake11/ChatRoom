#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
#include "ChatDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow; // Qt 设计器生成的类，负责管理界面元素. 与MainWindow不同。
}
QT_END_NAMESPACE


// 自定义的MainWindow类，继承自QMainWindow。
// 封装了Ui::MainWindow，方便交互。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void createRegisterDialog();
    void createResetDialog();
    void createChatDialog();
private:
    // 通过指针访问UI界面。
    Ui::MainWindow *ui;
    LoginDialog *_login_dialog;
    RegisterDialog *_register_dialog;
    ResetDialog *_reset_dialog;
	ChatDialog* _chat_dialog;

};
#endif // MAINWINDOW_H
