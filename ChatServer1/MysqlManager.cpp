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

int MysqlManager::updateLoginStatus(int uid, int status, const std::string& last_login)
{
	int res = _dao.updateLoginStatus(uid, status, last_login);
	if (res < 1) {
		std::cerr << "update login status failed, return " << res << std::endl;
	}
    return res;
}

std::unique_ptr<std::vector<MessageItem>> MysqlManager::getMessageList(int uid)
{
	return _dao.getMessageList(uid);
}

std::unique_ptr<std::vector<ContactItem>> MysqlManager::getContactList(int uid)
{
	return _dao.getContactList(uid);
}
