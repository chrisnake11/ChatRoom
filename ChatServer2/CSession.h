#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>
#include "Const.h"
#include "MsgNode.h"
#include "CServer.h"
#include "LogicSystem.h"

/*
CSession 类用于表示一个网络会话，封装了与客户端的连接、数据接收和发送等功能。

功能解析：
1. 构造函数和析构函数：
   - CSession()：初始化会话，设置默认值。
   - ~CSession()：清理资源，关闭连接。
2. 获取套接字和UUID：
   - getSocket()：返回与客户端的套接字引用。
   - getUuid()：返回会话的唯一标识符。
3. 会话管理：
   - start()：开始会话，通常用于启动异步操作。
   - close()：关闭会话，释放资源。
4. 异步操作：
   - asyncReadHead()：异步读取数据包头部，指定总长度。
   - asyncReadBody()：异步读取数据包体，指定总长度。
   - send()：发送数据到客户端。
5. 数据处理：
   - asyncReadFull()：异步读取完整数据包。
   - asyncReadLen()：异步读取指定长度的数据。

*/

class CSession : public std::enable_shared_from_this<CSession>{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	boost::asio::ip::tcp::socket& getSocket();
	std::string getSessionID() const { return _session_id; }
	int getUserID() const { return _user_id; }
	void setUserID(int id) { _user_id = id; }
	void start();
	void close();
	// 异步解析头部
	void asyncReadHead(int total_length);
	// 解析并处理数据包体
	void asyncReadBody(int total_length);
	// send data
	void send(const char* msg, short max_length, short msg_id);
	void send(const std::string& msg, short msg_id);
	// 支持右值引用，临时字符串对象传递。
	void send(std::string&& msg, short msg_id);

private:
	// 解析完整的数据包(head + body)
	// max_length表示头部长度
	// handler为回调函数
	void asyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code& error, std::size_t)> handler);
	// 封装async_read_some异步读取函数
	// 读取指定长度的数据，read_length为已处理的数据，total_length为包的总长度。
	void asyncReadLen(std::size_t read_length, std::size_t total_length, std::function<void(const boost::system::error_code& error, std::size_t)> handler);

	void handleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> self_shared);

	// server
	boost::asio::ip::tcp::socket _socket;
	CServer* _server;
	bool _b_close;

	// session id
	std::string _session_id;
	// user id
	int _user_id;

	// data, 单个消息的缓冲区
	char _data[MAX_LENGTH];

	// send data
	std::queue<std::shared_ptr<SendNode>> _send_queue;
	std::mutex _send_lock;
	

	bool _b_head_parse;
	// head data node
	std::shared_ptr<MsgNode> _recv_head_node;
	// msg data node
	std::shared_ptr<RecvNode> _recv_msg_node;
	
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
private:
	std::shared_ptr<CSession> _session;
	std::shared_ptr<RecvNode> _recv_node;
};