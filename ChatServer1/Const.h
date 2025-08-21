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
};

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
	MSG_CHAT_LOGIN_REQ = 1005,
	MSG_CHAT_LOGIN_RSP = 1006,
};

#define USER_TOKEN "usertoken_"
#define USER_IP "userip_"
#define IP_COUNT "ipcount_"
#define LOGIN_COUNT "logincount_"
#define USER_BASE_INFO "userbaseinfo_" 