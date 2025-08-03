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
	// ѭ��������Ϣ�����е���Ϣ
	while (true) {
		// ����
		std::unique_lock<std::mutex> lock(_node_mutex);
		// ������в�Ϊ�ջ��߷������ر��������
		while (_node_queue.empty() && !_b_stop) {
			_consume.wait(lock);
		}

		// ����������رգ�ѭ��������ݡ�
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
			// ��ն��к��˳�
			break;
		}

		// ���в�Ϊ�գ�ȡ��һ���ڵ���д���
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
	// ����ɿձ�Ϊ���գ������߳�
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
	// Json �����ַ�����ʽ��Ϣ
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	std::cout << "user login uid is: " << root["uid"].asInt() << 
		", user token is: " << root["token"].asString() << std::endl;

	// process data
	// ��¼��ȥStatusServer��ѯ�û�token�Ƿ���Ч
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

	// ChatServer�ı����ڴ��в�ѯ�û���Ϣ
	auto user_iter = _users.find(root["uid"].asInt());
	std::shared_ptr<UserInfo> user_info = nullptr;
	if (user_iter == _users.end()) {
		// �ڴ��Ҳ�����ȥ���ݿ��ѯ
		user_info = MysqlManager::getInstance()->getUser(root["uid"].asInt());
		if (user_info == nullptr) {
			return_root["error"] = ErrorCodes::ERROR_UID_INVALID;
			return;
		}
		// ���û���Ϣ�����ڴ�
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

