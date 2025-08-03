#pragma once
#include <boost/asio.hpp>
#include<boost/beast.hpp>
#include<boost/beast/http.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
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
	ERROR_JSON_PARSE_FAILED = 1001, // json����ʧ��
	ERROR_RPC_FAILED = 1002, // rpc����ʧ��
	ERROR_VARIFY_EXPIRED = 1003, // ��֤�����
	ERROR_VARIFY_WRONG = 1004, // ��֤�����
	ERROR_USER_EXIST = 1005, // �û��Ѵ���
	ERROR_PASSWORD_WRONG = 1006, // �������
	ERROR_EMAIL_NOT_MATCH = 1007, // ���䲻ƥ��
	ERROR_PASSWORD_UPDATE_FAILED = 1008, // �������ʧ��
	ERROR_PASSWORD_INVALID = 1009, // ������Ч 
	ERROR_UID_INVALID = 1010, // �û�ID��Ч
	ERROR_TOKEN_INVALID = 1011, // token��Ч
};

const std::string CODEPREFIX = "code_";


// Defer�࣬����ȷ��ĳ���������뿪����������ʱ����
// ʾ����
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


// DeferHelper�࣬����ȷ��ĳ���������뿪����������ʱ����
// T Ϊ���ƶ��ҿɵ��õĶ���
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

struct UserInfo {
	int uid;
	std::string name;
	std::string email;
	std::string passwd;
};


#define USERTOKENPREFIX "usertoken_"
#define LOGIN_COUNT  "logincount"