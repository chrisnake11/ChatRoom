#include "ChatServiceImpl.h"
#include "UserManager.h"
#include "CSession.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "RedisManager.h"
#include "MysqlManager.h"
#include "UserManager.h"
#include <memory>
#include "CSession.h"
#include "Const.h"

ChatServiceImpl::ChatServiceImpl() {
}

ChatServiceImpl::~ChatServiceImpl() {

}

Status ChatServiceImpl::notifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* response) {
	return Status::OK;
}

Status ChatServiceImpl::notifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response) {
	return Status::OK;

}

Status ChatServiceImpl::notifyTextChatMessage(ServerContext* context, const TextChatMessageReq* request, TextChatMessageRsp* response) {
	std::cout << "=== 收到notifyTextChatMessage请求 ===" << std::endl;
	std::cout << "发送者UID: " << request->uid() << std::endl;
	std::cout << "接收者UID: " << request->touid() << std::endl;
	std::cout << "消息数量: " << request->message_size() << std::endl;
	// 发送消息给当前服务器上的用户
	int friend_uid = request->touid();
	std::shared_ptr<CSession> session = UserManager::getInstance()->getSession(friend_uid);
	
	// 读取所有消息，逐个发送给friend的客户端
	for (auto& item : request->message()) {
		Json::Value return_value;
		return_value["error"] = 0;
		return_value["sender_id"] = request->uid();
		return_value["receiver_id"] = request->touid();
		return_value["message"] = item.data();
		session->send(return_value.toStyledString(), MSG_RECEIVE_CHAT_MESSAGE);
	}

	// 响应数据
	response->set_error(0);
	response->set_uid(request->uid());
    response->set_touid(request->touid());

	return Status::OK;
}
