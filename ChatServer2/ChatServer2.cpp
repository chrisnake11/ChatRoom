// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "ChatServiceImpl.h"
#include <iostream>

bool b_stop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
    std::cout << "Start ChatServer..." << std::endl;
    try {
        auto& config = ConfigManager::GetInstance();
        auto server_name = config["SelfServer"]["Name"];
        auto pool = AsioIOServicePool::getInstance();
		
        // 将当前ChatServer的登录人数初始化为0
		RedisManager::getInstance()->HSet(LOGIN_COUNT, server_name, "0");

        // define grpc server
		std::string grpc_server_address(config["SelfServer"]["Host"] + ":" + config["SelfServer"]["RPCPort"]);
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
		builder.AddListeningPort(grpc_server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		std::unique_ptr<grpc::Server> grpc_server(builder.BuildAndStart());
		std::cout << "Chat grpc-server listening on " << grpc_server_address << std::endl;

        // start a thread run grpc server
        std::thread grpc_server_thread([&grpc_server]() {
            grpc_server->Wait();
            });

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool, &grpc_server](auto, auto) {
            io_context.stop();
            pool->Stop();
			// 优雅退出grpc server
            grpc_server->Shutdown();
            });

        // cserver port
        auto port_str = config["SelfServer"]["Port"];
        CServer server(io_context, atoi(port_str.c_str()));
        io_context.run();

		// 退出删除当前ChatServer的登录人数记录
		RedisManager::getInstance()->HDel(LOGIN_COUNT, server_name);
		// 关闭RedisManager
		RedisManager::getInstance()->Close();
		// 关闭grpc线程
		grpc_server_thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
