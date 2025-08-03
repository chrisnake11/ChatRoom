#pragma once

#include <QObject>
#include "singleton.h"
#include <QString>

class UserManager : public QObject, public Singleton<UserManager>, public std::enable_shared_from_this<UserManager>
{
	Q_OBJECT
public:
    friend class Singleton<UserManager>;
    ~UserManager();
    void setName(QString name);
    void setUid(int uid);
    void setToken(QString token);
private:
    UserManager();
    QString _name;
    QString _token;
    int _uid;
};

