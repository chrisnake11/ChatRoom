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

// 聊天服务器的实现类，用于响应GRPC的添加好友，授权好友，发送文本消息等请求。
class ChatServiceImpl final : public ChatService::Service
{
public:
	ChatServiceImpl();
	~ChatServiceImpl();
	
	Status notifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* response) override;
	
	Status notifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response) override;
	
	// 响应文本消息，发送给对应的用户，并提醒有未读消息。
	Status notifyTextChatMessage(ServerContext* context, const TextChatMessageReq* request, TextChatMessageRsp* response) override;

};

