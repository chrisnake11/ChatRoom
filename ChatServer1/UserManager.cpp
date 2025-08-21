#include "UserManager.h"
#include "CSession.h"
#include "RedisManager.h"

UserManager::UserManager()
{
}

UserManager::~UserManager()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_user_sessions.clear();
}

std::shared_ptr<CSession> UserManager::getSession(int uid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _user_sessions.find(uid);
	if (it != _user_sessions.end()) {
		return it->second;
	}
	return nullptr;
}

void UserManager::setUserSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_user_sessions[uid] = session;
}

void UserManager::removeUserSession(int uid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_user_sessions.erase(uid);
}
