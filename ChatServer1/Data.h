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
	UserInfo();
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

// 聊天消息
struct MessageItem {
	MessageItem() : uid(0), unread_count(0) {}
	MessageItem(const int& uid, const std::string& nickname, const std::string& avatar, const std::string& message, const std::string& last_message_time, const int& unread_count)
		: uid(uid), nickname(nickname), avatar(avatar), message(message), last_message_time(last_message_time), unread_count(unread_count) {}
	int uid;
	std::string nickname;
	std::string avatar;
	std::string message;
	std::string last_message_time;
	int unread_count;
};

struct ContactItem {
	ContactItem() : uid(0), online_status(0) {}
	ContactItem(const int& uid, const std::string& nickname, const std::string& avatar, const std::string& sign, const int& online_status):
	uid(uid), nickname(nickname), avatar(avatar), sign(sign), online_status(online_status){ }
	int uid;	
	std::string nickname;
	std::string avatar;
	std::string sign;
	int online_status;
};

struct AddFriendInfo {
};

struct AuthFriendInfo {
};