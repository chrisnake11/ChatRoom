#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "Const.h"
#include "Singleton.h"
#include <atomic>
#include <queue>
#include <condition_variable>

using grpc::Channel; //
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class GrpcClientPool {
public:
	GrpcClientPool(std::size_t pool_size, std::string host, std::string port);
	~GrpcClientPool();

	void Close();
	// 消费者
	std::unique_ptr<VarifyService::Stub> GetStub();
	// 生产者
	void returnStub(std::unique_ptr<VarifyService::Stub> stub);
private:
	std::atomic<bool> _b_stop;
	size_t _pool_size;
	std::string _host;
	std::string _port;
	// Grpc信使队列
	std::queue<std::unique_ptr<VarifyService::Stub>> _stub_queue;
	std::mutex _stub_mutex; // stub队列的互斥锁
	std::condition_variable _stub_cond;
};

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	// 向邮箱发送验证码
	GetVarifyRsp GetVarifyCode(std::string email);
private:
	VerifyGrpcClient();

	// stub是一个客户端用来调用远程服务的代理。
	std::unique_ptr<GrpcClientPool> _stub_pool;

};

