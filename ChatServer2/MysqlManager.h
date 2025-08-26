#pragma once
#include "Const.h"
#include "MysqlDao.h"

// 单例类进一步封装MySQLDao类
class MysqlManager : public Singleton<MysqlManager>
{
	friend class Singleton<MysqlManager>;
public:
	~MysqlManager();
	int registerUser(const std::string& name, const std::string& email, const std::string& passwd);

	int resetUserPasswd(const std::string& email, const std::string& passwd);

	bool checkNameAndPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info);
private:
	MysqlManager();
	MysqlDao _dao;
};

