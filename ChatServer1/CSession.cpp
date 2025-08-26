#include "CSession.h"
#include <iostream>

CSession::CSession(boost::asio::io_context& io_context, CServer* server): _socket(io_context), _server(server), _b_close(false), _b_head_parse(false),_user_id(0)
{
	std::memset(_data, 0, MAX_LENGTH);
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_session_id = boost::uuids::to_string(a_uuid);
	_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LENGTH);
}

CSession::~CSession()
{
	std::cout << "~CSession()" << std::endl;
}

void CSession::start()
{
	asyncReadHead(HEAD_TOTAL_LENGTH);
}

void CSession::close() {
	_socket.close();
	_b_close = true;
}

boost::asio::ip::tcp::socket& CSession::getSocket() {
	return _socket;
}

void CSession::asyncReadFull(std::size_t max_length, std::function<void(const boost::system::error_code& error, std::size_t)> handler)
{
	std::cout << "AysncReadFull: " << max_length << std::endl;
	::memset(_data, 0, max_length);
	asyncReadLen(0, max_length, handler);
}

void CSession::asyncReadLen(std::size_t read_length, std::size_t total_length, std::function<void(const boost::system::error_code& error, std::size_t)> handler) {
	auto self = shared_from_this();
	_socket.async_read_some(boost::asio::buffer(_data + read_length, total_length - read_length), 
		[self, read_length, total_length, handler](const boost::system::error_code& error, std::size_t bytes_transfered) {
			// 出错，错误交给回调函数处理。
			if (error) {
				handler(error, read_length + bytes_transfered);
				return;
			}
			// 读取到的长度足够了，回调函数
			if (read_length + bytes_transfered >= total_length) {
				handler(error, read_length + bytes_transfered);
				return;
			}

			self->asyncReadLen(read_length + bytes_transfered, total_length, handler);
				
			
		});
}

void CSession::handleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> self_shared)
{
	try {
		if(error){
			std::cout << "handle write failed, error is " << error.what() << std::endl;
			close();
			_server->clearSession(_session_id);
		}
		std::lock_guard<std::mutex> lock(_send_lock);
		_send_queue.pop();
		if (!_send_queue.empty()) {
			auto& send_node = _send_queue.front();
			boost::asio::async_write(_socket, boost::asio::buffer(send_node->_data, send_node->_total_length), std::bind(&CSession::handleWrite, this, std::placeholders::_1, self_shared));
		}
	}
	catch (std::exception& e) {
		std::cout << "Exception code: " << e.what() << std::endl;
	}
}

void CSession::asyncReadHead(int total_len)
{
	auto self = shared_from_this();

	// 读取从(0, HEAD_TOTAL_LENGTH)的数据
	asyncReadFull(HEAD_TOTAL_LENGTH, [self, this](const boost::system::error_code& error, std::size_t bytes_transfered) {
		try {
			if (error) {
				std::cout << "handle read failed, error is: " << error.what() << std::endl;
				close();
				_server->clearSession(_session_id);
				return;
			}

			if (bytes_transfered < HEAD_TOTAL_LENGTH) {
				std::cout << "read head length not match, read [" << bytes_transfered << "] , total[" << HEAD_TOTAL_LENGTH << "] " << std::endl;
				close();
				_server->clearSession(_session_id);
				return;
			}

			_recv_head_node->clear();
			memcpy(_recv_head_node->_data, _data, bytes_transfered);

			// get MSGID
			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LENGTH);
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg id is " << msg_id;
			
			// msg_id 非法
			if (msg_id > MAX_LENGTH) {
				std::cout << "msg_id too long" << std::endl;
				_server->clearSession(_session_id);
				return;
			}

			short msg_len = 0;
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LENGTH, HEAD_DATA_LENGTH);
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is" << msg_len << std::endl;
			if (msg_len > MAX_LENGTH) {
				std::cout << "msg_len too long" << std::endl;
				_server->clearSession(_session_id);
				return;
			}

			// 构造RecvNode，开始读取消息体
			_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
			// 读取从0开始，长度为msg_len的数据
			asyncReadBody(msg_len);

		}
		catch (const std::exception& e) {
			// Handle exception
			std::cerr << "Exception in asyncReadHead: " << e.what() << std::endl;
		}
		});
}

// 读取从数据包payload部分，(0, total_length)的数据
void CSession::asyncReadBody(int total_length) {
	auto self = shared_from_this();
	asyncReadFull(total_length, [self, this, total_length](const boost::system::error_code& error, std::size_t bytes_transfered) {
		try {
			if (error) {
				std::cout << "handle read failed, error is " << error.what() << std::endl;
				close();
				_server->clearSession(_session_id);
				return;
			}
			// 保证一次读取完毕
			if (bytes_transfered < total_length) {
				std::cout << "read length not match read[" << bytes_transfered << "], total[" << total_length << "]" << std::endl;
				close();
				_server->clearSession(_session_id);
				return;
			}

			_recv_msg_node->clear();
			// 拷贝数据
			memcpy(_recv_msg_node->_data, _data, bytes_transfered);
			_recv_msg_node->_cur_length += bytes_transfered;
			_recv_msg_node->_data[_recv_msg_node->_total_length] = '\0';
			std::cout << "recv_msg_data is" << _recv_msg_node->_data << std::endl;

			// 构造LogicNode，放入消息队列
			LogicSystem::getInstance()->postMsgToQueue(std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
			
			// 当前消息处理完成，继续读取下一个消息头
			asyncReadHead(HEAD_TOTAL_LENGTH);

		}
		catch (std::exception& e) {
			std::cerr << "Exception code is" << e.what() << std::endl;
		}
		
	});
}

void CSession::send(const char* msg, short max_length, short msg_id) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_queue_size = _send_queue.size();
	if (_send_queue.size() >= MAX_SEND_QUEUE) {
		std::cout << "session:" << _session_id << ", send queue is full" << std::endl;
		return;
	}
	_send_queue.push(std::make_shared<SendNode>(msg, max_length, msg_id));
	if (_send_queue.size() > 1) {
		return;
	}
	auto& send_node = _send_queue.front();
	std::cout << "csession send_node: " << send_node->_data << std::endl;
	boost::asio::async_write(_socket, boost::asio::buffer(send_node->_data, send_node->_total_length), 
		std::bind(&CSession::handleWrite, this, std::placeholders::_1, shared_from_this()));
}

void CSession::send(const std::string& msg, short msg_id) {
	// 使用 c_str() 转换为 const char*
	send(msg.c_str(), static_cast<short>(msg.length()), msg_id);
}

// 支持移动语义的重载（C++11及以上）
void CSession::send(std::string&& msg, short msg_id) {
	send(msg.c_str(), static_cast<short>(msg.length()), msg_id);
}


LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recv_node): _session(session), _recv_node(recv_node)
{

}
