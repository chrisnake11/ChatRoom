#pragma once
#include <boost/asio.hpp>
#include<boost/beast.hpp>
#include<boost/beast/http.hpp>
#include <iostream>
#include <string>
#include <iomanip>
#include <memory>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <unordered_map>
#include <utility>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
using tcp = net::ip::tcp;

enum ErrorCodes {
	SUCCESS = 0,
	ERROR_JSON_PARSE_FAILED = 1001, // json解析失败
	ERROR_RPC_FAILED = 1002, // rpc调用失败
	ERROR_VARIFY_EXPIRED = 1003, // 验证码过期
	ERROR_VARIFY_WRONG = 1004, // 验证码错误
	ERROR_USER_EXIST = 1005, // 用户已存在
	ERROR_PASSWORD_WRONG = 1006, // 密码错误
	ERROR_EMAIL_NOT_MATCH = 1007, // 邮箱不匹配
	ERROR_PASSWORD_UPDATE_FAILED = 1008, // 密码更新失败
	ERROR_PASSWORD_INVALID = 1009, // 密码无效 
	ERROR_UID_INVALID = 1010, // 用户ID无效
	ERROR_TOKEN_INVALID = 1011, // token无效
	ERROR_LOAD_USERINFO_FAILED = 1012, // 用户登录失败
	ERROR_LOAD_FRIEND_INFO_FAILED = 1013, // 加载好友信息失败
	ERROR_GRPC_SEND_MESSAGE_ERROR = 1014, // grpc发送消息失败
};

// 验证码redis前缀
const std::string CODEPREFIX = "code_";


// Defer类，用于确保某个函数在离开作用域销毁时调用
// 示例：
// start scope
// {
//     Defer defer([&]() {
//         // do something
//     });
//     // do something when leave scope
// }
class Defer {
public:
	explicit Defer(std::function<void()> f) : _func(f) {}
	~Defer() { _func(); }
private:
	std::function<void()> _func;

};


// DeferHelper类，用于确保某个函数在离开作用域销毁时调用
// T 为可移动且可调用的对象
template<typename T>
class DeferHelper {
public:
	explicit DeferHelper(T t) : _t(std::move(t)) {}
	~DeferHelper() { _t(); }
private:
	T _t;
};

template<typename T>
DeferHelper<T> MakeDefer(T t) {
	return DeferHelper<T>(std::move(t));
};

#define MAX_LENGTH 2048
#define HEAD_TOTAL_LENGTH 4
#define HEAD_ID_LENGTH 2
#define HEAD_DATA_LENGTH 2
#define MAX_RECV_QUEUE 10000
#define MAX_SEND_QUEUE 10000

enum MSG_ID {
	MSG_CHAT_LOGIN = 1005, // 登录请求
	MSG_LOGOUT = 1006, // 登出
	MSG_GET_MESSAGE_LIST = 1007, // 获取消息列表
	MSG_GET_CHAT_MESSAGE = 1008, // 获取聊天消息
	MSG_GET_CONTACT_LIST = 1009, // 获取联系人列表
	MSG_GET_CONTACT_INFO = 1010, // 获取联系人信息
	MSG_SEND_CHAT_MESSAGE = 1011, // 发送聊天消息，响应
	MSG_RECEIVE_CHAT_MESSAGE = 1012, // 收到聊天消息
};

// redis前缀 key
#define USER_TOKEN "usertoken_" // 用户token前缀
#define USER_IP "userip_" // 用户ip前缀
#define IP_COUNT "ipcount_" // ip计数前缀
#define LOGIN_COUNT "logincount_" // 登录计数前缀
#define USER_BASE_INFO "userbaseinfo_" // 用户信息前缀
#define USER_MESSAGE_LIST "usermessagelist_" // 用户消息列表前缀
#define USER_CONTACT_LIST "usercontactlist_" // 用户联系人列表前缀
#define USER_CHAT_MESSAGE "userchatmessage_" // 用户聊天消息前缀