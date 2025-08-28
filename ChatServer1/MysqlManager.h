#pragma once
#include "Const.h"
#include "MysqlDao.h"


// 单例类进一步封装MySQLDao类
class MysqlManager : public Singleton<MysqlManager>
{
	friend class Singleton<MysqlManager>;
public:
	~MysqlManager();

	std::unique_ptr<UserInfo> getUserInfo(int uid);
	int updateLoginStatus(int uid, int status, const std::string& last_login);
	std::unique_ptr<std::vector<MessageItem>> getMessageList(int uid);
	std::unique_ptr<std::vector<ContactItem>> getContactList(int uid);
private:
	MysqlManager();
	MysqlDao _dao;
};

