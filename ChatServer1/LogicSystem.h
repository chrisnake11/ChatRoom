#pragma once
#include "Const.h"
#include "Singleton.h"
#include <map>
#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
class CSession;
class LogicNode;

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> node_callback;

class LogicSystem : public Singleton<LogicSystem>{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void postMsgToQueue(std::shared_ptr<LogicNode> msg_node);
private:
	LogicSystem();
	void registerHandler();
	void dealMsg();

	// ��¼�߼�����
	void loginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);

	// 
	bool _b_stop;

	// ���̴߳�����Ϣ�ڵ㡣
	std::mutex _node_mutex;
	std::queue<std::shared_ptr<LogicNode>> _node_queue;
	std::thread _work_thread;
	std::condition_variable _consume;

	// ��ͬ��Ϣ���߼������б�
	std::map <short, node_callback> _handlers;

	// �û��б�
	std::unordered_map<int, std::shared_ptr<UserInfo>> _users;
};