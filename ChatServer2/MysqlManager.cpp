#include "MysqlManager.h"

MysqlManager::~MysqlManager()
{

}

int MysqlManager::RegisterUser(const std::string& name, const std::string& email, const std::string& passwd)
{
	return _dao.RegisterUser(name, email, passwd);
}

int MysqlManager::ResetUser(const std::string& email, const std::string& passwd)
{
	return _dao.ResetUser(email, passwd);
}

bool MysqlManager::checkPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info)
{
	return _dao.checkPasswd(name, passwd, user_info);
}

std::shared_ptr<UserInfo> MysqlManager::getUser(int uid)
{
	return _dao.getUser(uid);
}

MysqlManager::MysqlManager()
{
}