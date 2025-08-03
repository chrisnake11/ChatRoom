#include "VerifyGrpcClient.h"
#include "ConfigManager.h"

VerifyGrpcClient::VerifyGrpcClient() {
	auto& config_manager = ConfigManager::GetInstance();
	auto host = config_manager["VarifyServer"]["Host"];
	auto port = config_manager["VarifyServer"]["Port"];
	_stub_pool.reset(new GrpcClientPool(5, host, port));
}

GetVarifyRsp VerifyGrpcClient::GetVarifyCode(std::string email) {
	// ����һ���ͻ���
	ClientContext context;

	// ����������Ϣ
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);

	// ͨ����������Զ�̷���
	auto stub = _stub_pool->GetStub();
	Status status = stub->GetVarifyCode(&context, request, &reply);

	if (status.ok()) {
		_stub_pool->returnStub(std::move(stub));
		return reply;
	}
	else {
		_stub_pool->returnStub(std::move(stub));
		reply.set_error(ErrorCodes::ERROR_RPC_FAILED);
		return reply;
	}
}

GrpcClientPool::GrpcClientPool(std::size_t pool_size, std::string host, std::string port):
	_pool_size(pool_size),
    _host(host),
    _port(port),
	_b_stop(false)
{
	for (std::size_t i = 0; i < pool_size; ++i) {
		// ����һ��ͨ������ͨ�� (��ַ����֤��ʽ)
		std::shared_ptr<Channel> channel = grpc::CreateChannel(
			host+":"+port,
			grpc::InsecureChannelCredentials()
		);
		// ����������һ��ͨ��
		_stub_queue.push(VarifyService::NewStub(channel));
	}
}

GrpcClientPool::~GrpcClientPool()
{
	std::lock_guard<std::mutex> lock(_stub_mutex);
	Close();
	while (!_stub_queue.empty()) {
		auto& stub = _stub_queue.front();
        stub.reset();
        _stub_queue.pop();
	}
}

void GrpcClientPool::Close() {
	_b_stop = true;
	_stub_cond.notify_all();
}

std::unique_ptr<VarifyService::Stub> GrpcClientPool::GetStub()
{
	std::unique_lock<std::mutex> lock(_stub_mutex);
	// �����ȴ������ֹͣ�����п��õ�stub�ͼ�����
    _stub_cond.wait(lock, [this] { return _b_stop || !_stub_queue.empty(); });
	// ���ֹͣ�ͷ���nullptr
	if (_b_stop) {
        return nullptr;
	}
	// ����Ӷ��з���stub
	auto stub = std::move(_stub_queue.front());
	_stub_queue.pop();
	return stub;
}

void GrpcClientPool::returnStub(std::unique_ptr<VarifyService::Stub> stub)
{
	std::lock_guard<std::mutex> lock(_stub_mutex);
	if (_b_stop) {
		return;
	}
    _stub_queue.push(std::move(stub));
    _stub_cond.notify_one();
}
