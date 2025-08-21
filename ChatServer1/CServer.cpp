#include "CServer.h"
#include "CSession.h"
#include <iostream>
#include "AsioIOServicePool.h"
#include "UserManager.h"

CServer::CServer(boost::asio::io_context& io_context, short port): _io_context(io_context), _port(port), _acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
	std::cout << "Chat Server init Success, listen on port: " << port << std::endl;
	startAccept();
}

CServer::~CServer()
{
	close();
}
void CServer::close() {
	// 关闭接受器
	boost::system::error_code ec;
	_acceptor.close(ec);
	if (ec) {
		std::cout << "Close acceptor error: " << ec.what() << std::endl;
	}

	// 清空sessions
	std::lock_guard<std::mutex> lock(_mutex);
	for (auto& pair : _sessions) {
		if (pair.second) {
			pair.second->close();
		}
	}
	_sessions.clear();
	std::cout << "server close..." << std::endl;

}

void CServer::startAccept() {
	std::cout << "server start accept..." << std::endl;
	auto& io_context = AsioIOServicePool::getInstance()->GetIOService();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->getSocket(), std::bind(&CServer::handleAccept, this, new_session, std::placeholders::_1));
}

void CServer::handleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	if (!error){
		std::cout << "session accept success, session is" << new_session.get() << std::endl;
		// start session
		new_session->start();
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.insert(std::make_pair(new_session->getSessionID(), new_session));
	}
	else {
		std::cout << "session accept failed, error is: " << error.what() << std::endl;
	}
}

void CServer::clearSession(std::string session_id) {
	if(_sessions.find(session_id) != _sessions.end()) {
		UserManager::getInstance()->removeUserSession(_sessions[session_id]->getUserID());
	}

	{
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.erase(session_id);
	}
}
