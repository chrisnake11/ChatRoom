#ifndef GLOBAL_H
#define GLOBAL_H

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QWidget>
#include <functional>
#include "QStyle"
#include <QByteArray>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QString>
#include <QCryptographicHash>


extern std::function<void(QWidget*)> repolish;

extern std::function<QString(QString)> md5Encrypt;

enum RequestID {
    ID_GET_VARIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_RESET_USER = 1003,
    ID_LOGIN_USER = 1004,
    ID_CHAT_LOGIN = 1005,
    ID_CHAT_LOGIN_RSP = 1006,
};

enum Modules{
    REG_MODEL = 0,
    RESET_MODEL = 1,
    LOGIN_MODEL = 2
};

enum ErrorCodes{
    SUCCESS = 0,
    ERROR_JSON_PARSE_FAILED = 1001, //Json 解析失败
    ERR_NETWORK = 2, // 网络或者服务异常
    ERROR_RPC_FAILED = 1002, // rpc调用失败
    ERROR_VARIFY_EXPIRED = 1003, // 验证码过期
    ERROR_VARIFY_WRONG = 1004, // 验证码错误
    ERROR_USER_EXIST = 1005, // 用户已存在
    ERROR_PASSWORD_WRONG = 1006, // 密码错误
    ERROR_EMAIL_NOT_MATCH = 1007, // 邮箱不匹配
    ERROR_PASSWORD_UPDATE_FAILED = 1008, // 密码更新失败
    ERROR_PASSWORD_INVALID = 1009, // 密码无效
};

enum TipErr {
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum ClickLabelState{
    NORMAL = 0,
    SELECTED = 1,
};

struct ServerInfo {
    quint16 uid;
    QString host;
    QString port;
    QString token;
};

extern QString gate_url_prefix;

#endif // GLOBAL_H
