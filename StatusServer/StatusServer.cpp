#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "ConfigManager.h"
#include "hiredis.h"
#include "RedisManager.h"
#include "MysqlManager.h"
#include "AsioIOServicePool.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "StatusServiceImpl.h"

void RunServer() {
	auto& cfg = ConfigManager::GetInstance();

	// grpc�����ĵ�ַ
	std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
	
	// grpc����ʵ��
	StatusServiceImpl service;

	// ����grpc������
	grpc::ServerBuilder builder;
	// �����˿ں���ӷ���
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	// ����������gRPC������
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;

	// ����Boost.Asio��io_context
	boost::asio::io_context io_context;
	// ����signal_set���ڲ���SIGINT�����ر�grpc������
	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
	signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
		if (!error) {
			std::cout << "Shutting down server..." << std::endl;
			server->Shutdown(); // ���ŵعرշ�����
		}
		});

	// �ػ��߳�����io_context������SIGINT�ź�
	std::thread([&io_context]() { io_context.run(); }).detach();

	// �ȴ�grpc�������ر�
	server->Wait();
	// ֹͣboost��io_context����
	io_context.stop();
}

int main(int argc, char** argv) {
	try {
		RunServer();
	}
	catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}