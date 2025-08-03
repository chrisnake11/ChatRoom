#include "global.h"

std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};

std::function<QString(QString)> md5Encrypt = [](const QString& input){
    QByteArray byteArray = input.toUtf8();
    QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5);
    return hash.toHex();
};

QString gate_url_prefix = "";
