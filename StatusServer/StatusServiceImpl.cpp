#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "const.h"
//#include "RedisManager.h"

std::string generate_unique_string() {
	// ����UUID����
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// ��UUIDת��Ϊ�ַ���
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
	// ��tokenд��map��
	insertToken(request->uid(), reply->token());
	return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const message::LoginReq* request, message::LoginRsp* response)
{
	/*
		�ڱ���map�в����û�ID��token�Ƿ�ƥ��
	*/
	auto uid = request->uid();
	auto token = request->token();
	std::lock_guard<std::mutex> lock(_server_mtx);
	auto iter = _uid_to_token_map.find(uid);
	// uidû�ҵ�
	if(iter == _uid_to_token_map.end()) {
		response->set_error(ErrorCodes::ERROR_UID_INVALID);
		return Status::OK;
	}
	// uid�ҵ�������token��ƥ��
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
	// ���token������map��
	std::lock_guard<std::mutex> lock(_token_mtx);
	_uid_to_token_map[uid] = token;
}


ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex> lock(_server_mtx);
	if (_token_to_server_map.empty()) {
		std::cerr << "No chat server available!" << std::endl;
		return ChatServer(); // ����һ���յ�ChatServer����
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
		��ȡ�����ļ��е���������������ã�����ӵ�_token_to_server_map��
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