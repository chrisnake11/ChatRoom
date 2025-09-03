#pragma once
#include "Const.h"
#include "Singleton.h"
#include <map>
#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <memory>
#include <vector>
#include "Data.h"


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

	// 用户发送消息逻辑处理
	void sendChatMessageHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	// 登录逻辑处理
	void loginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	// 登出逻辑处理
	void logoutHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& user_id);
	// 获取消息列表
	void messageListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	// 获取聊天消息列表
	void chatMessageListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	// 获取联系人列表
	void contactListHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);


	// 获取消息列表
	std::unique_ptr<std::vector<MessageInfo>> getMessageList(std::string list_key, int uid);
	// 获取聊天消息列表，同步Mysql和Redis缓存
	std::unique_ptr<std::vector<ChatMessageInfo>> getChatMessageList(const std::string& token_key, const int& uid, const int& friend_uid, const int& message_id = -1);
	// 获取联系人列表
	std::unique_ptr<std::vector<ContactInfo>> getContactList(const std::string& token_key, const int& uid);

	// 从redis中获取用户信息，或在MySQL中获取缓存到Redis中，读取到user_info
	// base_key为redis_key
	std::unique_ptr<UserInfo> getUserInfo(std::string base_key, int uid);

	// 
	bool _b_stop;

	// 单线程处理消息节点。
	std::mutex _node_mutex;
	std::queue<std::shared_ptr<LogicNode>> _node_queue;
	std::thread _work_thread;
	std::condition_variable _consume;

	// 不同消息的逻辑函数列表
	std::map <short, node_callback> _handlers;
};