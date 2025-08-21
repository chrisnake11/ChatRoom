#include "LogicSystem.h"
#include "CSession.h"
#include "StatusGrpcClient.h"
#include "MysqlManager.h"
#include "RedisManager.h"
#include "UserManager.h"

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
	_handlers[MSG_CHAT_LOGIN_REQ] = std::bind(&LogicSystem::loginHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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
		session->send(return_str, MSG_CHAT_LOGIN_RSP);
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

	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = getBaseInfo(base_key, uid, user_info);
	if(!b_base) {
		return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
		return;
	}

	return_root["uid"] = user_info->uid;
	return_root["passwd"] = user_info->passwd;
	return_root["name"] = user_info->name;
	return_root["email"] = user_info->email;
	return_root["sex"] = user_info->sex;
	return_root["avatar"] = user_info->avatar; 
	return_root["sign"] = user_info->sign;
	return_root["remark"] = user_info->remark;

	// 从数据库获取好友申请列表
	// 从数据库获取好友列表

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
	// Redis记录用户登录的ip server
	RedisManager::getInstance()->Set(ipkey, server_name);

	// uid和Session绑定，为了方便踢人
	UserManager::getInstance()->setUserSession(uid, session);

	return;
}

bool LogicSystem::getBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info) {
	// 从Redis查询用户基本信息
	std::string base_value = "";
	bool success = RedisManager::getInstance()->Get(base_key, base_value);
	if (success) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(base_value, root);
		user_info->uid = root["uid"].asInt();
		user_info->passwd = root["passwd"].asString();
		user_info->name = root["name"].asString();
		user_info->email = root["email"].asString();
		user_info->avatar = root["avatar"].asString();
		user_info->sign = root["sign"].asString();
		user_info->remark = root["remark"].asString();
		user_info->sex = root["sex"].asInt();
	}
	// 从Mysql查询用户基本信息
	else {
		std::shared_ptr<UserInfo> tmp_user_info = nullptr;
		tmp_user_info = MysqlManager::getInstance()->getUser(uid);
		if(tmp_user_info == nullptr) {
			// 用户不存在
			return false;
		}

		user_info = tmp_user_info;
	
		// 写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = user_info->uid;
		redis_root["passwd"] = user_info->passwd;
		redis_root["name"] = user_info->name;
		redis_root["email"] = user_info->email;
		redis_root["sex"] = user_info->sex;
		redis_root["avatar"] = user_info->avatar;
		redis_root["sign"] = user_info->sign;
		redis_root["remark"] = user_info->remark;
		// Json序列化到Redis中
		RedisManager::getInstance()->Set(base_key, redis_root.toStyledString());
	}
	return true;
}

