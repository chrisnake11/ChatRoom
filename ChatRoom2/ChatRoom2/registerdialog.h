#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "global.h"
#include <QMap>

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    // Http请求返回后的回调函数
    void on_reg_mod_finish(RequestID id, QString res, ErrorCodes err);
    // 获取验证码
    void on_get_code_btn_clicked();

    void on_register_btn_clicked();


signals:
    // 切换到登陆界面
    void switch_to_login();
    void register_success();

private:
    Ui::RegisterDialog *ui;

    // 显示提示信息
    void showTip(QString msg, bool success);

    // 注册Handler回调函数
    void initHttpHandlers();

    // 根据请求类型，读取回调函数
    QMap<RequestID, std::function<void(const QJsonObject&)>> _handlers;

    bool checkNameValid();
    bool checkEmailValid();
    bool checkPasswdValid();
    bool checkConfirmValid();
    bool checkCodeValid();

    // 存储错误信息
    QMap<TipErr, QString> _tip_errs;
    void addTipErr(TipErr err, QString tips);
    void delTipErr(TipErr err);

};

#endif // REGISTERDIALOG_H
