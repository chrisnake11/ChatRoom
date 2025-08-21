#pragma once

#include "Const.h"
#include "Singleton.h"
#include "ConfigManager.h"
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <queue>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Data.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerReq;
using message::LoginReq;
using message::LoginRsp;
using message::ChatService;

using message::TextChatMessageReq;
using message::TextChatMessageRsp;
using message::TextChatData;

// Ϊ��ʹ����������ܹ���Ч����������ӣ�ʹ�����ӳ�
class ChatConPool {
public:
	ChatConPool(size_t pool_size, std::string host, std::string port)
		: _pool_size(pool_size), _host(std::move(host)), _port(std::move(port)) {
		// �������GRPC����
		for (size_t i = 0; i < _pool_size; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());
			_connections.push(ChatService::NewStub(channel));
		}
	}
	~ChatConPool() {
		std::lock_guard<std::mutex> lock(_mutex);
		Close();
		while(!_connections.empty()) {
			_connections.pop();
		}
	}

	void Close() {
		_b_stop = true;
		_cond.notify_all();
	}

	std::unique_ptr<ChatService::Stub> getConnection() {
		std::unique_lock<std::mutex> lock(_mutex);
		// ֱֹͣ�ӷ��ؿ�
		if (_b_stop) {
			return nullptr;
		}

		// ûֹͣ����Ϊ�գ��ȴ�
		while (_connections.empty() && !_b_stop) {
			_cond.wait(lock);
		}
		
		// ûֹͣ����Ϊ�գ���������
		std::unique_ptr<ChatService::Stub> conn = std::move(_connections.front());
		_connections.pop();
		return conn;
	}

	void returnConnection(std::unique_ptr<ChatService::Stub> conn) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (_b_stop) {
			return;
		}
		_connections.push(std::move(conn));
		_cond.notify_one();
	}

private:
	std::atomic<bool> _b_stop;
	size_t _pool_size;
	std::string _host;
	std::string _port;
	std::queue<std::unique_ptr<ChatService::Stub>> _connections;
	std::mutex _mutex;
	std::condition_variable _cond;
};

class ChatGrpcClient : public Singleton<ChatGrpcClient> {
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient() {

	}

	// ����GRPC�ӿڣ�����1Ϊ������IP������2Ϊ�������ݸ�ʽ
	AddFriendRsp notifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp notifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool getBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info);
	TextChatMessageRsp notifyTextChatMessage(std::string server_ip, const TextChatMessageReq& req, const Json::Value& return_value);
	

private:
	ChatGrpcClient();
	std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};