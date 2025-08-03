#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <unordered_map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

// ChatServer类，用地址和端口表示
class ChatServer {
public:
	ChatServer() : host(""), port(""), name(""), con_count(0) {}
	ChatServer(const ChatServer& server)
		: host(server.host), port(server.port), name(server.name), con_count(server.con_count) {
	}

	ChatServer& operator=(const ChatServer& server) {
		if (this != &server) {
			host = server.host;
			port = server.port;
			name = server.name;
			con_count = server.con_count;
		}
		return *this;
	}
	std::string host;
	std::string port;
	std::string name;
	int con_count;
};

class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl();

	// 获取聊天服务器信息，添加到本地map中
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* response) override;

	Status Login(ServerContext* context, const message::LoginReq* request,
		message::LoginRsp* response) override;

private:
	void insertToken(int uid, const std::string& token);

	// 从本地map中获取聊天服务器信息
	ChatServer getChatServer();

	std::unordered_map<int, std::string> _uid_to_token_map; // 存储用户ID到token的映射
	std::mutex _token_mtx; // 用于保护_token_to_server_map的互斥锁
	std::unordered_map<std::string, ChatServer> _token_to_server_map;
	std::mutex _server_mtx;
};