#pragma once
#include <string>
#include <memory>
#include <json/value.h>
struct BaseUserInfo {
	BaseUserInfo() : uid(0), username(""), email(""), passwd("") {}
	int uid;
	std::string username;
	std::string email;
	std::string passwd;
};

struct UserInfo : public BaseUserInfo {
public:
	UserInfo() : BaseUserInfo(),avatar(""), sign(""), nickname(""), gender(0), birthday(""), phone(""), address(""), online_status(0), last_login(""), register_time("") {}
	std::string nickname; // 昵称
	std::string phone; // 电话
	std::string address; // 地址
	std::string avatar; // 头像
	int gender; // 性别
	std::string birthday; // 生日
	std::string sign; // 个性签名
	int online_status; // 在线状态，0-离线，1-在线
	std::string last_login; // 最后登录时间
	std::string register_time; // 注册时间

	// json转UserInfo
	static void loadFromJson(std::unique_ptr<UserInfo>& user_info, const Json::Value& root);
	// UserInfo转Json
	static void convertToJson(std::unique_ptr<UserInfo>& user_info, Json::Value& root);
};

struct AddFriendInfo {
};

struct AuthFriendInfo {
};