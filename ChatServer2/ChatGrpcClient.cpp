#include "ChatGrpcClient.h"
#include "UserManager.h"
#include "CSession.h"
#include "ConfigManager.h"

ChatGrpcClient::ChatGrpcClient()
{
	// 读取所有的聊天服务器地址
	auto& config = ConfigManager::GetInstance();
	auto server_list = config["PeerServer"]["Servers"];

	// 将聊天服务器地址字符串转为数组
	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	// 给每个聊天服务器创建一个连接池
	for (auto& word : words) {
		if (config[word]["Name"].empty()) {
			continue;
		}
		_pools[config[word]["Name"]] = std::make_unique<ChatConPool>
			(5, config[word]["Host"], config[word]["RPCPort"]);
	}

}

// 调用GRPC接口，参数1为服务器IP，参数2为请求数据格式
AddFriendRsp ChatGrpcClient::notifyAddFriend(std::string server_ip, const AddFriendReq& req) {
	// TODO
	AddFriendRsp rsp;
	return rsp;
}
AuthFriendRsp ChatGrpcClient::notifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
	// TODO
	AuthFriendRsp rsp;
	return rsp;
}
TextChatMessageRsp ChatGrpcClient::notifyTextChatMessage(std::string server_name, const TextChatMessageReq& req) {
	// 选取指定的服务器的连接
	ClientContext context;
	TextChatMessageRsp reply;
	auto stub = _pools[server_name]->getConnection();

	// 调用GRPC接口
    Status status = stub->notifyTextChatMessage(&context, req, &reply);
	Defer defer([&stub, &server_name, this] {
		_pools[server_name]->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		reply.set_error(ErrorCodes::ERROR_RPC_FAILED);
	}

	return reply;
}
