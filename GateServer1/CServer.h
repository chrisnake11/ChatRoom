#pragma once

#include "HttpConnection.h"
#include "Const.h"

class CServer : public std::enable_shared_from_this<CServer>
{
public:
	explicit CServer(boost::asio::io_context& ioc, unsigned short& port);
	CServer(const CServer&) = delete;
    CServer& operator=(const CServer&) = delete;
	void Start();

private:
	net::io_context& _ioc;
	tcp::acceptor _acceptor;
};

