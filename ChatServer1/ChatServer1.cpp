// ChatServer1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigManager.h"
// #include "LogicSystem.h"

#include <iostream>

bool b_stop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
    std::cout << "Start ChatServer..." << std::endl;
    try {
        auto& config = ConfigManager::GetInstance();
        auto pool = AsioIOServicePool::getInstance();
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool](auto, auto) {
            io_context.stop();
            pool->Stop();
            });
        auto port_str = config["SelfServer"]["Port"];
        CServer server(io_context, atoi(port_str.c_str()));
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
