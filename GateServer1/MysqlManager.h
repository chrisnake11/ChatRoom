#pragma once
#include "Const.h"
#include "MysqlDao.h"

// �������һ����װMySQLDao��
class MysqlManager : public Singleton<MysqlManager>
{
	friend class Singleton<MysqlManager>;
public:
	~MysqlManager();
	int RegisterUser(const std::string& name, const std::string& email, const std::string& passwd);

	int ResetUser(const std::string& email, const std::string& passwd);

	bool checkPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info);
private:
	MysqlManager();
	MysqlDao _dao;
};

