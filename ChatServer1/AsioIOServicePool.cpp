#include "AsioIOServicePool.h"

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
	auto& service = _io_services[_next_io_service++];
	// 轮询。
	if (_next_io_service == _io_services.size()) {
		_next_io_service = 0;
	}
	return service;
}

void AsioIOServicePool::Stop()
{
	// 结束io_context
	for (auto& work : _works) {
		work.reset();
	}

	// 等待线程退出
	for (auto& thread : _threads) {
		thread.join();
	}
}

AsioIOServicePool::AsioIOServicePool(std::size_t size):_io_services(size),_works(size), _next_io_service(0) {
	// 初始化io_context
	for (std::size_t i = 0; i < size; i++) {
		_works[i] = std::unique_ptr<Work>(new Work(_io_services[i]));
	}

	// 为每个io_context创建一个线程，并且开始执行。
	for (std::size_t i = 0; i < size; i++) {
		_threads.emplace_back(std::thread(
			// this捕获，以访问_io_services[i]成员
			[this, i]() {
				_io_services[i].run();
			}
		));
	}
}
