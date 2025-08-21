#pragma once
#include <grpcpp/grpcpp.h>
#include "message.pb.h"
#include "message.grpc.pb.h"
#include <mutex>
#include "Data.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::TextChatMessageReq;
using message::TextChatMessageRsp;
using message::TextChatData;

class ChatServiceImpl final : public ChatService::Service
{
public:
	ChatServiceImpl();
	~ChatServiceImpl();
	
	Status notifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* response) override;
	
	Status notifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response) override;
	
	Status notifyTextChatMessage(ServerContext* context, const TextChatMessageReq* request, TextChatMessageRsp* response) override;
	
	// 查询用户信息
	bool getBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info);

};

