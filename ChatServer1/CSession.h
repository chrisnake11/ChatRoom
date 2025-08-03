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
CSession �����ڱ�ʾһ������Ự����װ����ͻ��˵����ӡ����ݽ��պͷ��͵ȹ��ܡ�

���ܽ�����
1. ���캯��������������
   - CSession()����ʼ���Ự������Ĭ��ֵ��
   - ~CSession()��������Դ���ر����ӡ�
2. ��ȡ�׽��ֺ�UUID��
   - getSocket()��������ͻ��˵��׽������á�
   - getUuid()�����ػỰ��Ψһ��ʶ����
3. �Ự����
   - start()����ʼ�Ự��ͨ�����������첽������
   - close()���رջỰ���ͷ���Դ��
4. �첽������
   - asyncReadHead()���첽��ȡ���ݰ�ͷ����ָ���ܳ��ȡ�
   - asyncReadBody()���첽��ȡ���ݰ��壬ָ���ܳ��ȡ�
   - send()���������ݵ��ͻ��ˡ�
5. ���ݴ���
   - asyncReadFull()���첽��ȡ�������ݰ���
   - asyncReadLen()���첽��ȡָ�����ȵ����ݡ�

*/

class CSession : public std::enable_shared_from_this<CSession>{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	boost::asio::ip::tcp::socket& getSocket();
	std::string& getUuid();
	void start();
	void close();
	// �첽����ͷ��
	void asyncReadHead(int total_length);
	// �������������ݰ���
	void asyncReadBody(int total_length);
	// send data
	void send(const char* msg, short max_length, short msg_id);
	void send(const std::string& msg, short msg_id);
	// ֧����ֵ���ã���ʱ�ַ������󴫵ݡ�
	void send(std::string&& msg, short msg_id);

private:
	// �������������ݰ�(head + body)
	// max_length��ʾͷ������
	// handlerΪ�ص�����
	void asyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code& error, std::size_t)> handler);
	// ��װasync_read_some�첽��ȡ����
	// ��ȡָ�����ȵ����ݣ�read_lengthΪ�Ѵ�������ݣ�total_lengthΪ�����ܳ��ȡ�
	void asyncReadLen(std::size_t read_length, std::size_t total_length, std::function<void(const boost::system::error_code& error, std::size_t)> handler);

	void handleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> self_shared);

	// server
	boost::asio::ip::tcp::socket _socket;
	CServer* _server;
	bool _b_close;

	// session uid
	std::string _uuid;

	// data, ������Ϣ�Ļ�����
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