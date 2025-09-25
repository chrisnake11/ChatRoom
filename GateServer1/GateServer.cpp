#include <iostream>
#include "CServer.h"
#include "Const.h"
#include "ConfigManager.h"
#include "AsioIOServicePool.h"
//#include <sw/redis++/redis++.h>
#include <hiredis/hiredis.h>
#include "RedisManager.h"
#include <cassert>

void TestRedisManager() {
    std::string value = "";
    assert(RedisManager::getInstance()->Set("blogwebsite", "llfc.club"));
    assert(RedisManager::getInstance()->Get("blogwebsite", value));
    assert(RedisManager::getInstance()->Get("nonekey", value) == false);
    assert(RedisManager::getInstance()->HSet("bloginfo", "blogwebsite", "llfc.club"));
    assert(RedisManager::getInstance()->HGet("bloginfo", "blogwebsite") != "");
    assert(RedisManager::getInstance()->ExistKey("bloginfo"));
    assert(RedisManager::getInstance()->Del("bloginfo"));
    assert(RedisManager::getInstance()->Del("bloginfo"));
    assert(RedisManager::getInstance()->ExistKey("bloginfo") == false);
    assert(RedisManager::getInstance()->LPush("lpushkey1", "lpushvalue1"));
    assert(RedisManager::getInstance()->LPush("lpushkey1", "lpushvalue2"));
    assert(RedisManager::getInstance()->LPush("lpushkey1", "lpushvalue3"));
    assert(RedisManager::getInstance()->RPop("lpushkey1", value));
    assert(RedisManager::getInstance()->RPop("lpushkey1", value));
    assert(RedisManager::getInstance()->LPop("lpushkey1", value));
    assert(RedisManager::getInstance()->LPop("lpushkey2", value) == false);
    std::cout << "Redis Test Finished..." << std::endl;
    //RedisManager::getInstance()->Close();
}

int main()
{
    //TestRedisManager();
    // 读取配置文件
    auto & config_manager = ConfigManager::getInstance();
    std::string gate_port_str = config_manager["GateServer"]["Port"];
    unsigned short gate_port = static_cast<unsigned short>(std::stoi(gate_port_str));
    
    try {
        net::io_context ioc{1};
        // 监听信号
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            // 停止服务
            AsioIOServicePool::getInstance()->Stop();
            });

        std::cout << "GateServer Start..." << std::endl;
        // 重要：先创建shared_ptr，再调用Start()
        std::make_shared<CServer>(ioc, gate_port)->Start();


        ioc.run();
    }

    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}