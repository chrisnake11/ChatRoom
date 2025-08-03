#include "LogicSystem.h"
#include "CSession.h"
#include "StatusGrpcClient.h"
#include "MysqlManager.h"

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
	_handlers[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::loginHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::loginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
	// Json 解析字符串格式消息
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	std::cout << "user login uid is: " << root["uid"].asInt() << 
		", user token is: " << root["token"].asString() << std::endl;

	// process data
	// 登录先去StatusServer查询用户token是否有效
	auto rsp = StatusGrpcClient::getInstance()->Login(root["uid"].asInt(), root["token"].asString());

	Json::Value return_root;
	Defer defer([this, &return_root, session]() {
		std::string return_str = return_root.toStyledString();
		session->send(return_str, MSG_CHAT_LOGIN);
		});
	
	return_root["error"] = rsp.error();
	if (return_root["error"] != ErrorCodes::SUCCESS) {
		return;
	}

	// ChatServer的本地内存中查询用户信息
	auto user_iter = _users.find(root["uid"].asInt());
	std::shared_ptr<UserInfo> user_info = nullptr;
	if (user_iter == _users.end()) {
		// 内存找不到，去数据库查询
		user_info = MysqlManager::getInstance()->getUser(root["uid"].asInt());
		if (user_info == nullptr) {
			return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
			return;
		}
		// 将用户信息存入内存
		_users[root["uid"].asInt()] = user_info;
	}
	else {
		user_info = user_iter->second;
	}

	return_root["uid"] = user_info->uid;
	return_root["token"] = rsp.token();
	return_root["name"] = user_info->name;

	// response data
	std::string return_str = return_root.toStyledString();
	session->send(return_str, msg_id);
}

