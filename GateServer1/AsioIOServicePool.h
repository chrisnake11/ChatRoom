#pragma once

#include "Singleton.h"
#include <thread>
#include <vector>
#include <boost/asio.hpp>

class AsioIOServicePool : public Singleton<AsioIOServicePool> {
	friend class Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using Work = boost::asio::io_context::work;
	using WorkPtr = std::unique_ptr<Work>;
	~AsioIOServicePool() { std::cout << "~AsioIOServicePool()" << std::endl; }
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;

	// 轮询获取一个io_context
	boost::asio::io_context& GetIOService();
	void Stop();
private:
	AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
	std::vector<IOService> _io_services;
	std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;
	std::size_t _next_io_service; // 下标
};