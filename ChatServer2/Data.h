#pragma once
#include <string>

class UserInfo {
public:
	UserInfo() : uid(0), sex(0), name(""), email(""), passwd(""), avatar(""), sign(""), remark("") {}
	int uid;
	std::string name;
	std::string email;
	std::string passwd;
	int sex;
	std::string avatar; // 头像
	std::string sign; // 个性签名
	std::string remark; // 备注
};

class AddFriendInfo {
};

class AuthFriendInfo {
};