#include "Data.h"
#include <iostream>

UserInfo::UserInfo() : BaseUserInfo(), avatar(""), sign(""), nickname(""), gender(0), birthday(""), phone(""), address(""), online_status(0), last_login(""), register_time("") {}

void UserInfo::loadFromJson(std::unique_ptr<UserInfo>& user_info, const Json::Value& root)
{
    if (!user_info) {
        user_info = std::make_unique<UserInfo>();
    }
    user_info->uid = root["uid"].asInt();
    user_info->username = root["username"].asString();
    user_info->email = root["email"].asString();
    user_info->passwd = root["passwd"].asString();
    user_info->nickname = root["nickname"].asString();
    user_info->gender = root["gender"].asInt();
    user_info->birthday = root["birthday"].asString();
    user_info->phone = root["phone"].asString();
    user_info->address = root["address"].asString();
    user_info->avatar = root["avatar"].asString();
    user_info->online_status = root["online_status"].asInt();
    user_info->last_login = root["last_login"].asString();
    user_info->register_time = root["register_time"].asString();
    user_info->sign = root["sign"].asString();
}


void UserInfo::convertToJson(std::unique_ptr<UserInfo>& user_info, Json::Value& root)
{
    if (!user_info) {
        user_info = std::make_unique<UserInfo>();
    }
    root["uid"] = user_info->uid;
    root["username"] = user_info->username;
    root["email"] = user_info->email;
    root["passwd"] = user_info->passwd;
    root["nickname"] = user_info->nickname;
    root["gender"] = user_info->gender;
    root["birthday"] = user_info->birthday;
    root["phone"] = user_info->phone;
    root["address"] = user_info->address;
    root["avatar"] = user_info->avatar;
    root["sign"] = user_info->sign;
    root["online_status"] = user_info->online_status;
    root["last_login"] = user_info->last_login;
    root["register_time"] = user_info->register_time;
}
