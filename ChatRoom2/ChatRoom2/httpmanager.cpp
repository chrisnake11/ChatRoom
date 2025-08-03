#include "httpmanager.h"


HttpManager::~HttpManager()
{
    std::cout << "~HttpManager()" << std::endl;
}

HttpManager::HttpManager()
{
    connect(this, &HttpManager::sig_http_finish, this, &HttpManager::slot_http_finish);
}

void HttpManager::postHttpRequest(QUrl url, QJsonObject json, RequestID req_id, Modules mod)
{
    // 读取并构建url和data数据
    QByteArray data = QJsonDocument(json).toJson();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.size()));
    auto self = shared_from_this();

    // 调用QNetworkAccessManager.post()发送请求
    QNetworkReply * reply = _manager.post(request, data);

    // 异步等待接收的数据，设置回调函数
    QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod]{
        // error
        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();
            emit self->sig_http_finish(req_id, " ", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        // success
        QString res = reply->readAll();
        // 发送http结束信号，让前端响应。
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
    });
}


void HttpManager::slot_http_finish(RequestID req_id, QString res, ErrorCodes err, Modules mod)
{
    // 根据ID，发送注册结束的信号
    if(mod == Modules::REG_MODEL){
        emit sig_reg_mod_finish(req_id, res, err);
    }
    if (mod == Modules::RESET_MODEL) {
        emit sig_reset_mod_finish(req_id, res, err);
    }

    if (mod == Modules::LOGIN_MODEL) {
        emit sig_login_mod_finish(req_id, res, err);
    }
}


