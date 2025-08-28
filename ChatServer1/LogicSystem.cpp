#include "LogicSystem.h"
#include "CSession.h"
#include "StatusGrpcClient.h"
#include "MysqlManager.h"
#include "RedisManager.h"
#include "UserManager.h"
#include "Utils.h"

LogicSystem::LogicSystem() : _b_stop(false) {
	registerHandler();
	_work_thread = std::thread(&LogicSystem::dealMsg, this);

}

LogicSystem::~LogicSystem() {
	std::cout << "LogicSystem destroy~" << std::endl;
	_b_stop = true;
	_consume.notify_one();
	_work_thread.join();
}

void LogicSystem::dealMsg() {
	std::shared_ptr<LogicNode> node;
	// 循环处理消息队列中的消息
	while (true) {
		// 加锁
		std::unique_lock<std::mutex> lock(_node_mutex);
		// 如果队列不为空或者服务器关闭则继续。
		while (_node_queue.empty() && !_b_stop) {
			_consume.wait(lock);
		}

		// 如果服务器关闭，循环清空数据。
		if (_b_stop) {
			while (!_node_queue.empty()) {
				auto node = _node_queue.front();
				auto msg_id = node->_recv_node->_msg_id;
				std::cout << "recv_msg id is " << node->_recv_node->_msg_id << std::endl;
				auto callback_iter = _handlers.find(msg_id);
				if (callback_iter == _handlers.end()) {
					_node_queue.pop();
					std::cout << "msg_id " << msg_id << " handler not found." << std::endl;
					continue;
				}
				callback_iter->second(node->_session, node->_recv_node->_msg_id, 
					std::string(node->_recv_node->_data, node->_recv_node->_cur_length));
			}
			// 清空队列后，退出
			break;
		}

		// 队列不为空，取出一个节点进行处理
		auto node = _node_queue.front();
		auto msg_id = node->_recv_node->_msg_id;
		std::cout << "logicSystem msg_id is: " << msg_id << std::endl;
		auto callback_iter = _handlers.find(msg_id);
		if (callback_iter == _handlers.end()) {
			_node_queue.pop();
			std::cout << "msg_id " << msg_id << " handler not found." << std::endl;
			continue;
		}
		callback_iter->second(node->_session, node->_recv_node->_msg_id,
			std::string(node->_recv_node->_data, node->_recv_node->_cur_length));

		_node_queue.pop();
	}
}

void LogicSystem::postMsgToQueue(std::shared_ptr<LogicNode> node) {
	std::unique_lock<std::mutex> lock(_node_mutex);
	_node_queue.push(node);
	// 如果由空变为不空，唤醒线程
	if (_node_queue.size() == 1) {
		lock.unlock();
		_consume.notify_one();
	}
}

void LogicSystem::registerHandler()
{
	_handlers[MSG_ID::MSG_CHAT_LOGIN] = std::bind(&LogicSystem::loginHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_handlers[MSG_ID::MSG_LOGOUT] = std::bind(&LogicSystem::logoutHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_handlers[MSG_ID::MSG_GET_MESSAGE_LIST] = std::bind(&LogicSystem::messageListHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_handlers[MSG_ID::MSG_GET_CONTACT_LIST] = std::bind(&LogicSystem::contactListHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::contactListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	Json::Value return_root;
	Defer defer([this, &return_root, session, &msg_id] {
		std::string return_str = return_root.toStyledString();
		// 调用session发送消息,消息码 MSG_GET_CONTACT_LIST = 1009
		session->send(return_str, msg_id);
		});

	if (!reader.parse(msg_data, root)) {
		std::cout << "parse json failed" << std::endl;
		return;
	}
	std::cout << "get contact list request " << std::endl;

	int uid = root["uid"].asInt();
	// 验证用户token
	std::string token_key = USER_TOKEN + root["uid"].asString();
	std::string token_value = "";
	bool get_token_success = RedisManager::getInstance()->Get(token_key, token_value);

	// token验证错误
	if (!get_token_success) {
		return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
		std::cerr << "redis get token failed" << std::endl;
	}
	if (token_value != root["token"].asString()) {
		return_root["error"] = ErrorCodes::ERROR_TOKEN_INVALID;
		std::cerr << "token invalid" << std::endl;
	}

	// token验证成功
	return_root["error"] = ErrorCodes::SUCCESS;
	std::string list_key = USER_CONTACT_LIST + root["uid"].asString();
	std::unique_ptr<std::vector<ContactItem>> contact_list = getContactList(list_key, uid);

	// 列表查询失败
	if (contact_list == nullptr) {
		return_root["error"] = ErrorCodes::ERROR_LOAD_FRIEND_INFO_FAILED;
		std::cerr << "ERROR_LOAD_FRIEND_CONTACT_INFO_FAILED" << std::endl;
		return;
	}
	
	// 添加联系人数组到Json数据中
	return_root["contact_list"] = Json::arrayValue;
    for (auto& item : (*contact_list)) { 
		Json::Value item_root;
        item_root["uid"] = item.uid;
        item_root["nickname"] = item.nickname;
        item_root["avatar"] = item.avatar;
        item_root["sign"] = item.sign;
        item_root["online_status"] = item.online_status;
        return_root["contact_list"].append(item_root);
	}


}

std::unique_ptr<std::vector<ContactItem>> LogicSystem::getContactList(const std::string& token_key, const int& uid) {
	std::string list_value = "";
	Json::Reader list_reader;
	Json::Value list_root;
	std::unique_ptr<std::vector<ContactItem>> list_ptr = std::make_unique<std::vector<ContactItem>>();
	bool success = RedisManager::getInstance()->Get(token_key, list_value);
	if (success) {
		if (!list_reader.parse(list_value, list_root)) {
			std::cout << "parse json failed" << std::endl;
		}
		for (const Json::Value& item : list_root) {
			ContactItem item_data;
			item_data.uid = item["uid"].asInt();
            item_data.nickname = item["nickname"].asString();
            item_data.avatar = item["avatar"].asString();
            item_data.sign = item["sign"].asString();
			item_data.online_status = item["online_status"].asInt();
			list_ptr->push_back(item_data);
		}
	}
	else {
		// 从MySQL获取列表
		list_ptr = MysqlManager::getInstance()->getContactList(uid);
		if (list_ptr == nullptr) {
			std::cout << "MySQL get contact list uid: " << uid << " failed. " << std::endl;
			return nullptr;
		}

		// 存储到Redis
		Json::Value root = root;
		for (auto& item : *(list_ptr)) {
			root.append(Json::Value(Json::objectValue));
			root[root.size() - 1]["uid"] = item.uid;
			root[root.size() - 1]["nickname"] = item.nickname;
            root[root.size() - 1]["avatar"] = item.avatar;
            root[root.size() - 1]["sign"] = item.sign;
            root[root.size() - 1]["online_status"] = item.online_status;
		}
		RedisManager::getInstance()->Set(token_key, root.toStyledString());
	}
	return std::move(list_ptr);
}

void LogicSystem::messageListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	// 获取用户的所有朋友消息
	Json::Reader reader;
	Json::Value root;
	// 定义返回的Json对象
	Json::Value return_root;
	// 函数退出时，发送返回return_root
	Defer defer([this, &return_root, session, &msg_id]() {
		std::string return_str = return_root.toStyledString();
		// 调用session发送消息,消息码 MSG_GET_MESSAGE_LIST = 1007
		session->send(return_str, msg_id);
		});

    if (!reader.parse(msg_data, root)) {
		std::cout << "parse json failed" << std::endl;
		return;
	}
	std::cout << "get message list request " << std::endl;
	
	int uid = root["uid"].asInt();
	// 验证用户token
	std::string token_key = USER_TOKEN + root["uid"].asString();
	std::string token_value = "";
	bool get_token_success = RedisManager::getInstance()->Get(token_key, token_value);

	// token验证错误
	if (!get_token_success) {
		return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
		std::cerr << "redis get token failed" << std::endl;
	}
	if (token_value != root["token"].asString()) {
		return_root["error"] = ErrorCodes::ERROR_TOKEN_INVALID;
		std::cerr << "token invalid" << std::endl;
	}

	// token验证成功
	return_root["error"] = ErrorCodes::SUCCESS;

	// 从redis中获取列表信息
    std::string list_key = USER_MESSAGE_LIST + std::to_string(uid);
	std::unique_ptr<std::vector<MessageItem>> message_list = getMessageList(list_key, uid);

	if (!message_list) {
		return_root["error"] = ErrorCodes::ERROR_LOAD_FRIEND_INFO_FAILED;
		std::cerr << "ERROR_LOAD_FRIEND_MESSAGE_INFO_FAILED" << std::endl;
		return;
	}

	// 创建 JSON 数组
	return_root["message_list"] = Json::arrayValue;
	// 将message数组添加到返回Json中
	for (const auto& item : *message_list) {
		Json::Value json_item;
		json_item["uid"] = item.uid;
		json_item["nickname"] = item.nickname;
		json_item["avatar"] = item.avatar;
		json_item["message"] = item.message;
		json_item["last_message_time"] = item.last_message_time;
		json_item["unread_count"] = item.unread_count;
		return_root["message_list"].append(json_item);
	}

}

std::unique_ptr<std::vector<MessageItem>> LogicSystem::getMessageList(std::string list_key, int uid) {
    std::string list_value = "";
	bool success = RedisManager::getInstance()->Get(list_key, list_value);
	Json::Reader list_reader;
	Json::Value list_root;
	std::unique_ptr<std::vector<MessageItem>> list_ptr = std::make_unique<std::vector<MessageItem>>();
	// 缓存命中
	if (success) {
		if (!list_reader.parse(list_value, list_root)) {
            std::cout << "parse json failed" << std::endl;
		}
        for (const Json::Value& item : list_root) {
			MessageItem msg;
			msg.uid = item["uid"].asInt();
			msg.nickname = item["nickname"].asString();
			msg.avatar = item["avatar"].asString();
            msg.message = item["message"].asString();
            msg.last_message_time = item["last_message_time"].asString();
            msg.unread_count = item["unread_count"].asInt();
            list_ptr->emplace_back(msg);
        }
	}
	// 去MySQL中获取列表信息
	else {
		// 获取uid对应的所有消息列表
		list_ptr = MysqlManager::getInstance()->getMessageList(uid);

		if (list_ptr == nullptr) {
			std::cerr << " MySQL get uid " << uid << " message list failed" << std::endl;
			return nullptr;
		}
		
		// 添加到Redis缓存
		Json::Value root;
        for (const auto& item : *list_ptr) {
            root.append(Json::Value(Json::objectValue));
            root[root.size() - 1]["uid"] = item.uid;
            root[root.size() - 1]["avatar"] = item.avatar;
            root[root.size() - 1]["nickname"] = item.nickname;
            root[root.size() - 1]["message"] = item.message;
            root[root.size() - 1]["last_message_time"] = item.last_message_time;
			root[root.size() - 1]["unread_count"] = item.unread_count;
        }
		RedisManager::getInstance()->Set(list_key, root.toStyledString());
	}
	return std::move(list_ptr);
}

void LogicSystem::logoutHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& user_id) {
	int uid = std::atoi(user_id.c_str());
	// 更新用户登录状态
	int res = MysqlManager::getInstance()->updateLoginStatus(uid, 0, "");
	if (res <= 0) {
		std::cout << "log out failed" << std::endl;
		return;
	}
	std::cout << "user uid: " << uid << " log out" << std::endl;
}


void LogicSystem::loginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	
	// Json 解析字符串格式消息
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	std::cout << "user login uid is: " << root["uid"].asInt() << 
		", user token is: " << root["token"].asString() << std::endl;

	// process data
	Json::Value return_root;
	Defer defer([this, &return_root, session]() {
		std::string return_str = return_root.toStyledString();
		// 调用session发送消息,消息码 MSG_CHAT_LOGIN = 1005
		session->send(return_str, MSG_CHAT_LOGIN);
		});

	// 从Redis查询用户token是否有效
	int uid = root["uid"].asInt();
	std::string uid_str = root["uid"].asString();
	std::string token_key = USER_TOKEN + uid_str;
	std::string token_value = "";
	bool success = RedisManager::getInstance()->Get(token_key, token_value);

	if(!success){
		return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
		return;
	}
	if(token_value != root["token"].asString()) {
		return_root["error"] = ErrorCodes::ERROR_TOKEN_INVALID;
		return;
	}

	return_root["error"] = ErrorCodes::SUCCESS;

	// Redis 读取用户信息
	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = getUserInfo(base_key, uid);
	// 返回空指针，用户不存在
	if(!user_info) {
		return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
		return;
	}

	// 更新登录状态和最后登录时间（同步MySQL和Redis）
	std::string current_time = getDateTimeStr();
	if (MysqlManager::getInstance()->updateLoginStatus(uid, 1, current_time)) {
		// 如果更新成功，则更新Redis中用户信息
		RedisManager::getInstance()->HSet(base_key, "online_status", "1");
		RedisManager::getInstance()->HSet(base_key, "last_login", current_time);
	}

	// 将用户信息添加到Json结果中。
	Json::Value user_json;
	UserInfo::convertToJson(user_info, user_json);
	// 将user_info作为对象，存储在Json结果中
	return_root["user_info"] = user_json;

	// 将uid和token添加到Json结果中
	return_root["uid"] = root["uid"];
	return_root["token"] = root["token"];


	// 更新redis中，ChatRoom的连接数量
	auto server_name = ConfigManager::GetInstance()["SelfServer"]["Name"];
	auto count_res = RedisManager::getInstance()->HGet(LOGIN_COUNT, server_name);
	int count = 0;
	if(!count_res.empty()) {
		count = std::stoi(count_res);
	}
	count++;
	auto count_str = std::to_string(count);
	RedisManager::getInstance()->HSet(LOGIN_COUNT, server_name, count_str);

	// session绑定用户ID
	session->setUserID(uid);

	// 为用户设置登录server的名字
	std::string ipkey = USER_IP + uid_str;

	// Redis记录用户登录的ip - server
	RedisManager::getInstance()->Set(ipkey, server_name);

	// uid和Session绑定，为了管理session
	UserManager::getInstance()->setUserSession(uid, session);

	std::cout << "LogicSystem return_root user_info: " << return_root.toStyledString() << std::endl;
}

std::unique_ptr<UserInfo> LogicSystem::getUserInfo(std::string base_key, int uid) {
	// 从Redis查询用户基本信息
	std::string base_value = "";
	bool success = RedisManager::getInstance()->Get(base_key, base_value);
	Json::Value root;
	Json::Reader reader;
	// 缓存命中
	if (success) {
		// 解析Json
        if (reader.parse(base_value, root)) {
			std::unique_ptr<UserInfo> user_info = std::make_unique<UserInfo>();
			UserInfo::loadFromJson(user_info, root);
			return user_info;
		}
		return nullptr;
	}
	// 从Mysql查询用户基本信息
	else {
		std::unique_ptr<UserInfo> user_info = MysqlManager::getInstance()->getUserInfo(uid);
		if(user_info == nullptr) {
			// 用户不存在
			return nullptr;
		}
		// 序列化成Json
        UserInfo::convertToJson(user_info, root);
		// 写入redis缓存
		RedisManager::getInstance()->Set(base_key, root.toStyledString());
		return user_info;
	}
}

