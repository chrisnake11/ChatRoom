#include "MysqlManager.h"

MysqlManager::MysqlManager()
{
}

MysqlManager::~MysqlManager()
{

}

std::unique_ptr<UserInfo> MysqlManager::getUserInfo(int uid)
{
	return _dao.getUserInfo(uid);
}