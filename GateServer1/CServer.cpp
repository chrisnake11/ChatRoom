#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):
	_ioc(ioc), _acceptor(ioc, net::ip::tcp::endpoint(tcp::v4(), port))
{
}

void CServer::Start() {
	auto self = shared_from_this();
	// ÿ�δ�io_context���л�ȡio_context��������socket
	boost::asio::io_context& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	// ����һ��HttpConnection����_socket�ϵ�IO�¼�
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
			// ��������
			self->Start();
		}
		catch (std::exception& e) {
			std::cout << "Exception in DoAccept: " << e.what() << std::endl;
		}
		});
}