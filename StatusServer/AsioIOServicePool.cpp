#include "AsioIOServicePool.h"

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
	auto& service = _io_services[_next_io_service++];
	// ��ѯ��
	if (_next_io_service == _io_services.size()) {
		_next_io_service = 0;
	}
	return service;
}

void AsioIOServicePool::Stop()
{
	// ����io_context
	for (auto& work : _works) {
		work.reset();
	}

	// �ȴ��߳��˳�
	for (auto& thread : _threads) {
		thread.join();
	}
}

AsioIOServicePool::AsioIOServicePool(std::size_t size):_io_services(size),_works(size), _next_io_service(0) {
	// ��ʼ��io_context
	for (std::size_t i = 0; i < size; i++) {
		_works[i] = std::unique_ptr<Work>(new Work(_io_services[i]));
	}

	// Ϊÿ��io_context����һ���̣߳����ҿ�ʼִ�С�
	for (std::size_t i = 0; i < size; i++) {
		_threads.emplace_back(std::thread(
			// this�����Է���_io_services[i]��Ա
			[this, i]() {
				_io_services[i].run();
			}
		));
	}
}
