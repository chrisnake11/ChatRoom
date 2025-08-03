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

	// grpc监听的地址
	std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
	
	// grpc服务实例
	StatusServiceImpl service;

	// 创建grpc服务器
	grpc::ServerBuilder builder;
	// 监听端口和添加服务
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	// 构建并启动gRPC服务器
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;

	// 创建Boost.Asio的io_context
	boost::asio::io_context io_context;
	// 创建signal_set用于捕获SIGINT，并关闭grpc服务器
	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
	signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
		if (!error) {
			std::cout << "Shutting down server..." << std::endl;
			server->Shutdown(); // 优雅地关闭服务器
		}
		});

	// 守护线程运行io_context，捕获SIGINT信号
	std::thread([&io_context]() { io_context.run(); }).detach();

	// 等待grpc服务器关闭
	server->Wait();
	// 停止boost的io_context服务
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