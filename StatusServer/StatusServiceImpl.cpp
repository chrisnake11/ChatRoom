#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "const.h"
//#include "RedisManager.h"

std::string generate_unique_string() {
	// 创建UUID对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// 将UUID转换为字符串
	std::string unique_string = to_string(uuid);

	return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("status server has receivec... ");
	const auto& server = getChatServer();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::SUCCESS);
	reply->set_token(generate_unique_string());
	// 将token写入map中
	insertToken(request->uid(), reply->token());
	return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const message::LoginReq* request, message::LoginRsp* response)
{
	/*
		在本地map中查找用户ID和token是否匹配
	*/
	auto uid = request->uid();
	auto token = request->token();
	std::lock_guard<std::mutex> lock(_server_mtx);
	auto iter = _uid_to_token_map.find(uid);
	// uid没找到
	if(iter == _uid_to_token_map.end()) {
		response->set_error(ErrorCodes::ERROR_UID_INVALID);
		return Status::OK;
	}
	// uid找到，但是token不匹配
	if (iter->second != token) {
		response->set_error(ErrorCodes::ERROR_TOKEN_INVALID);
		return Status::OK;
	}

	response->set_error(ErrorCodes::SUCCESS);
	response->set_uid(uid);
	response->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, const std::string& token)
{
	// 添加token到本地map中
	std::lock_guard<std::mutex> lock(_token_mtx);
	_uid_to_token_map[uid] = token;
}


ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex> lock(_server_mtx);
	if (_token_to_server_map.empty()) {
		std::cerr << "No chat server available!" << std::endl;
		return ChatServer(); // 返回一个空的ChatServer对象
	}
	auto min_server = _token_to_server_map.begin()->second;
	for(const auto& server : _token_to_server_map) {
		if(server.second.con_count < min_server.con_count) {
			min_server = server.second;
		}
	}
	return min_server;
}

StatusServiceImpl::StatusServiceImpl()
{
	/*
		读取配置文件中的聊天服务器的配置，并添加到_token_to_server_map中
	*/
	auto& cfg = ConfigManager::GetInstance();

	std::vector<std::string> server_names;

	ChatServer server;
	server.port = cfg["ChatServer1"]["Port"];
	server.host = cfg["ChatServer1"]["Host"];
	server.name = cfg["ChatServer1"]["Name"];
	server.con_count = 0;
	_token_to_server_map[server.name] = server;

}