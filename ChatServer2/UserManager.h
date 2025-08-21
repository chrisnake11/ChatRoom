#pragma once

#include "Singleton.h"
#include "Const.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class CSession;

/*
	ʹ��һ��map�������û�ID��session��ӳ���ϵ
*/

class UserManager : public Singleton<UserManager> {
	friend class Singleton<UserManager>;
public:
	~UserManager();
	std::shared_ptr<CSession> getSession(int uid);
	void setUserSession(int uid, std::shared_ptr<CSession> session);
	void removeUserSession(int uid);
private:
	UserManager();
	std::unordered_map<int, std::shared_ptr<CSession>> _user_sessions;
	std::mutex _mutex;
};