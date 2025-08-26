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
private:
	MysqlManager();
	MysqlDao _dao;
};

