#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QString>
#include "global.h"
#include <QMap>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    void showTip(QString str, bool success);

public slots:
    void on_login_btn_clicked();
    void on_forget_label_clicked();
    void slot_login_mod_finish(RequestID id, QString res, ErrorCodes err);
    void slot_tcp_connect_finish(bool success);
    void slot_login_failed(int error);

signals:
    void switch_to_register();
    void switch_to_reset();
    void switch_to_chat();
    void sig_connect_tcp(ServerInfo);

private:
    Ui::LoginDialog *ui;

    // 根据请求类型，读取回调函数
    QMap<RequestID, std::function<void(const QJsonObject&)>> _handlers;
    // 注册Handler回调函数
    void initHttpHandlers();
    quint16 _uid;
    QString _token;

    bool checkNameValid();
    bool checkPasswdValid();

    QMap<TipErr, QString> _tip_errs;
    void addTipErr(TipErr err, QString tips);
    void delTipErr(TipErr err);
};

#endif // LOGINDIALOG_H
