#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <map>
#include <mutex>

class CSession;

class CServer {
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	void clearSession(std::string);
	void close();

private:
	void handleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
	void startAccept();
	boost::asio::io_context &_io_context;
	short _port;
	boost::asio::ip::tcp::acceptor _acceptor;
	std::map < std::string, std::shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
};