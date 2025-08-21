#include "ChatGrpcClient.h"
#include "UserManager.h"
#include "CSession.h"
#include "ConfigManager.h"

ChatGrpcClient::ChatGrpcClient()
{
	// ��ȡ���е������������ַ
	auto& config = ConfigManager::GetInstance();
	auto server_list = config["PeerServer"]["Servers"];

	// �������������ַ�ַ���תΪ����
	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	// ��ÿ���������������һ�����ӳ�
	for (auto& word : words) {
		if (config[word]["Name"].empty()) {
			continue;
		}
		_pools[config[word]["Name"]] = std::make_unique<ChatConPool>
			(5, config[word]["Host"], config[word]["Port"]);
	}

}

// ����GRPC�ӿڣ�����1Ϊ������IP������2Ϊ�������ݸ�ʽ
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
TextChatMessageRsp ChatGrpcClient::notifyTextChatMessage(std::string server_ip, const TextChatMessageReq& req, const Json::Value& return_value) {
	// TODO
	TextChatMessageRsp rsp;
	return rsp;
}
