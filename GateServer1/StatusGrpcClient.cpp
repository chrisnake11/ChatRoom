#include "StatusGrpcClient.h"
#include "StatusGrpcClient.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = _pool->getConnection();
	Status status = stub->GetChatServer(&context, request, &reply);
	Defer defer([&stub, this]() {
		_pool->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::ERROR_RPC_FAILED);
		return reply;
	}
}

LoginRsp StatusGrpcClient::Login(int uid, const std::string& token)
{
	ClientContext context;
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);

	auto stub = _pool->getConnection();

	// 向StatusServer验证uid和token
	Status status = stub->Login(&context, request, &reply);

	Defer defer([&stub, this]() {
		_pool->returnConnection(std::move(stub));
		});

	if (status.ok()) {
		return reply;
	}

	else {
		reply.set_error(ErrorCodes::ERROR_RPC_FAILED);
		return reply;
	}
}

StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = ConfigManager::GetInstance();
	std::string host = gCfgMgr["StatusServer"]["Host"];
	std::string port = gCfgMgr["StatusServer"]["Port"];
	_pool.reset(new StatusConPool(5, host, port));
}