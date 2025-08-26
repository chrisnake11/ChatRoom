#include "MysqlManager.h"

MysqlManager::~MysqlManager()
{

}

int MysqlManager::registerUser(const std::string& name, const std::string& email, const std::string& passwd)
{
	// 1. 检查用户名和邮箱是否存在
	if (!_dao.existUserByName(name)) {
		return ERROR_USER_EXIST;
	} 
	if(!_dao.existUserByEmail(email)) {
		return ERROR_EMAIL_NOT_MATCH;
	}
	// 2. 更新用户id
	int uid = _dao.updateUserId();
	if (uid < 0) {
		return ERROR_USER_EXIST;
	}
	// 3. 创建用户基本信息
	BaseUserInfo base_info;
	base_info.uid = uid;
	base_info.username = name;
	base_info.email = email;
	base_info.passwd = passwd;
	int ret = _dao.createBaseUserInfo(base_info);
	if (ret < 0) {
		return ERROR_USER_EXIST;
	}
	return uid;
}

int MysqlManager::resetUserPasswd(const std::string& passwd, const std::string& email)
{
	if (_dao.existUserByEmail(email)) {
		return ERROR_EMAIL_NOT_MATCH;
	}

	int res = _dao.updateUserPasswdByEmail(passwd, email);
	if (res < 0) {
		return ERROR_EMAIL_NOT_MATCH;
	}
	return res;
}

bool MysqlManager::checkNameAndPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info)
{
	return _dao.checkNameAndPasswd(name, passwd, user_info);
}

MysqlManager::MysqlManager()
{
}