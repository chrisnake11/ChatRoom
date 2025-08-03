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
    assert(RedisManager::GetInstance()->Set("blogwebsite", "llfc.club"));
    assert(RedisManager::GetInstance()->Get("blogwebsite", value));
    assert(RedisManager::GetInstance()->Get("nonekey", value) == false);
    assert(RedisManager::GetInstance()->HSet("bloginfo", "blogwebsite", "llfc.club"));
    assert(RedisManager::GetInstance()->HGet("bloginfo", "blogwebsite") != "");
    assert(RedisManager::GetInstance()->ExistKey("bloginfo"));
    assert(RedisManager::GetInstance()->Del("bloginfo"));
    assert(RedisManager::GetInstance()->Del("bloginfo"));
    assert(RedisManager::GetInstance()->ExistKey("bloginfo") == false);
    assert(RedisManager::GetInstance()->LPush("lpushkey1", "lpushvalue1"));
    assert(RedisManager::GetInstance()->LPush("lpushkey1", "lpushvalue2"));
    assert(RedisManager::GetInstance()->LPush("lpushkey1", "lpushvalue3"));
    assert(RedisManager::GetInstance()->RPop("lpushkey1", value));
    assert(RedisManager::GetInstance()->RPop("lpushkey1", value));
    assert(RedisManager::GetInstance()->LPop("lpushkey1", value));
    assert(RedisManager::GetInstance()->LPop("lpushkey2", value) == false);
    std::cout << "Redis Test Finished..." << std::endl;
    //RedisManager::GetInstance()->Close();
}

int main()
{
    //TestRedisManager();
    // 读取配置文件
    auto & config_manager = ConfigManager::GetInstance();
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
            AsioIOServicePool::GetInstance()->Stop();
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