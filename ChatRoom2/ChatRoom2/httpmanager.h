#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include "global.h"
#include "singleton.h"

class HttpManager: public QObject, public Singleton<HttpManager>, public std::enable_shared_from_this<HttpManager>
{
    Q_OBJECT
public:
    ~HttpManager();
    // url：Http请求
    // json：发送的数据
    // req_id：请求的类型
    // mod：发送的模块类型
    void postHttpRequest(QUrl url, QJsonObject json, RequestID req_id, Modules mod);
private:
    friend class Singleton<HttpManager>;
    HttpManager();
    QNetworkAccessManager _manager;

private slots:
    // 根据事件类型，让对应的前端响应
    void slot_http_finish(RequestID req_id, QString res, ErrorCodes err, Modules mod);

signals:
    // http接收完成信号。
    void sig_http_finish(RequestID req_id, QString res, ErrorCodes err, Modules mod);

    // 注册界面的信号
    void sig_reg_mod_finish(RequestID req_id, QString res, ErrorCodes err);

    // 重置界面的信号
    void sig_reset_mod_finish(RequestID req_id, QString res, ErrorCodes err);

    // 登录界面的信号
    void sig_login_mod_finish(RequestID req_id, QString res, ErrorCodes err);

};

#endif // HTTPMANAGER_H
