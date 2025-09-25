#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):
	_ioc(ioc), _acceptor(ioc, net::ip::tcp::endpoint(tcp::v4(), port))
{
}

void CServer::Start() {
	auto self = shared_from_this();
	// 每次从io_context池中获取io_context，并创建socket
	boost::asio::io_context& io_context = AsioIOServicePool::getInstance()->GetIOService();
	// 创建一个HttpConnection处理_socket上的IO事件
	auto http_connection = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(http_connection->GetSocket(), [self, http_connection](beast::error_code ec) {
		try {
			if (ec) {
				// accept other socket
				self->Start();
				// discard this socket
				return;
			}
			http_connection->Start();
			// 继续监听
			self->Start();
		}
		catch (std::exception& e) {
			std::cout << "Exception in DoAccept: " << e.what() << std::endl;
		}
		});
}