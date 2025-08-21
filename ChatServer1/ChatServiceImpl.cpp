#include "ChatServiceImpl.h"
#include "UserManager.h"
#include "CSession.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "RedisManager.h"
#include "MysqlManager.h"

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
	return Status::OK;

}

bool ChatServiceImpl::getBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info) {
	return false;
}
