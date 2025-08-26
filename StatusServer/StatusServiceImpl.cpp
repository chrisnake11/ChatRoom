#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "const.h"
#include "RedisManager.h"

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
	// 获取连接数最小的服务器
	const auto& server = getChatServer();
	// 返回服务器地址和端口
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::SUCCESS);
	reply->set_token(generate_unique_string());
	// 将token写入Redis
	insertToken(request->uid(), reply->token());
	return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const message::LoginReq* request, message::LoginRsp* response)
{
	/*
		在Redis中查找用户ID和token是否匹配
	*/
	auto uid = request->uid();
	auto token = request->token();

	// redis查询uid对应的token, key-value
	std::string token_str = "";
	bool success = RedisManager::getInstance()->Get(USER_TOKEN + std::to_string(uid), token_str);
	// uid不存在，查询失败
	if (!success) {
		response->set_error(ErrorCodes::ERROR_UID_INVALID);
		return Status::OK;
	}
	// 比较登录的token和redis的token是否匹配
	if (token_str != token) {
		response->set_error(ErrorCodes::ERROR_TOKEN_INVALID);
		return Status::OK;
	}

	// redis查询成功，token匹配
	response->set_error(ErrorCodes::SUCCESS);
	response->set_uid(uid);
	response->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, const std::string& token)
{
	std::string uid_str = std::to_string(uid);
	std::string key = USER_TOKEN + uid_str;
	// 写入redis，key-value
	RedisManager::getInstance()->Set(key, token);
}


ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex> lock(_server_mtx);
	if (_servers.empty()) {
		std::cout << "In status server, server list is empty" << std::endl;
		// 服务器列表为空，返回默认服务器
		ChatServer default_server;
		default_server.con_count = INT_MAX;
		return default_server;
	}

	auto min_server = _servers.begin()->second;
	// redis查询第一个的连接数
	auto count_str = RedisManager::getInstance()->HGet(LOGIN_COUNT, min_server.name);
	if (count_str.empty()) {
		// redis查询失败，设置为最大值
		min_server.con_count = INT_MAX;
	}
	else {
		min_server.con_count = std::stoi(count_str);
	}

	// 找到最小的元素
	for(auto& server : _servers) {
		// 跳过自己
		if(server.second.name == min_server.name) {
			continue;
		}
		// redis查询连接数
		auto count_str = RedisManager::getInstance()->HGet(LOGIN_COUNT, server.second.name);
		if(count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

		// 连接数更小，更新
		if(server.second.con_count < min_server.con_count) {
			min_server = server.second;
		}
	}
	
	// 服务器都满了
	if(min_server.con_count == INT_MAX) {
		// 可以报错提示
		std::cout << "all servers are full connected" << std::endl;
	}
	// 更新连接数加1
	else{
		RedisManager::getInstance()->HSet(LOGIN_COUNT, min_server.name, std::to_string(min_server.con_count + 1));
	}
	return min_server;
}

StatusServiceImpl::StatusServiceImpl()
{
	/*
		读取配置文件中的聊天服务器的配置，并添加到_servers中
	*/
	auto& cfg = ConfigManager::GetInstance();

	auto server_list = cfg["ChatServers"]["Name"];
	std::cout << "server list: " << server_list << std::endl;

	// 按逗号分割字符串
	std::stringstream ss(server_list);
	std::vector<std::string> words;
	std::string word;
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}
	for (auto& word : words) {
		ChatServer server;
		server.name = cfg[word]["Name"];
		server.host = cfg[word]["Host"];
		server.port = cfg[word]["Port"];
		_servers[server.name] = server;
		std::cout << "parse configuration. server name: " << server.name << ", host: " << server.host << ", port: " << server.port << std::endl;
	}


}