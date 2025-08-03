#include "UserManager.h"

UserManager::UserManager() : _name(""), _token(""), _uid(0)
{
}

UserManager::~UserManager()
{}

void UserManager::setName(QString name)
{
	_name = name;
}

void UserManager::setUid(int uid)
{
	_uid = uid;
}

void UserManager::setToken(QString token)
{
	_token = token;
}



