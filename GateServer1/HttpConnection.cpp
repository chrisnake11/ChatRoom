#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(net::io_context& io_context)
:_socket(io_context), _deadline(_socket.get_executor(), std::chrono::seconds(60)) {

	// !!! 不要再构造函数内部调用
	// Start();
}

void HttpConnection::Start() {
    std::cout << "Starting connection" << std::endl;
	auto self = shared_from_this();
	// 读取请求到beast::flat_buffer中，自动解析为beast::http::request
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transfered) {
		try {
			if(ec){
				std::cout << "http read Error: " << ec.message() << std::endl;
			}

			//  无需处理粘包
			boost::ignore_unused(bytes_transfered);
			// 处理请求
			self->HandleRequest();
			self->CheckDeadline();
			self->WriteResponse();
		}
		catch (std::exception& e) {
			std::cout << "http read Error: " << e.what() << std::endl;
		}
		});
}

tcp::socket& HttpConnection::GetSocket()
{
	return _socket;
}

void HttpConnection::CheckDeadline()
{
	auto self = shared_from_this();
	_deadline.async_wait([self](beast::error_code ec) {
		// 响应超时
		if (!ec) {
			self->_deadline.cancel();
		}
	});
}


// 注：一个字符串10,需要2个ASCII码存储
// 将0~15的ASCII字符转换成16进制。
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

// 将16进制的ASCII字符转换成0~15的ASCII字符。
unsigned char FromHex(unsigned char x) {
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

/*
	将URL中的字符串从string类型编码为char数字流。
	或者将char数字流，解码为url字符串。
*/

// 将url字符串，编码为char数字流。
std::string UrlEncode(const std::string& str) {
	std::string str_temp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++) {
		// 如果是数字或者字母，以及特殊字符
		if (isalnum(unsigned char(str[i]))
			|| (str[i] == '-')
			|| (str[i] == '~')
			|| (str[i] == '.')
			|| (str[i] == '_'))
		{
			str_temp += str[i];
		}
		// 处理空字符
		else if (str[i] == ' ') {
			str_temp += '+';
		}
		else {
			// 其他字符先加%，分别把高8位和低8位转换成16进制
			str_temp += '%';
			str_temp += ToHex((unsigned char)str[i] >> 4); // 高8位
			str_temp += ToHex((unsigned char)str[i] & 0x0F); // 低8位
		}
	}
	return str_temp;
}

// 从URL中解码
std::string UrlDecode(const std::string& str) {
	std::string str_temp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++) {
		// 处理空格
		if (str[i] == '+') str_temp += ' ';
		// 处理百分号编码
		else if (str[i] == '%') {
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			str_temp += high * 16 + low;
		}
		else {
			str_temp += str[i];
		}
	}
	return str_temp;
}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transfered) {
		// 发送结束，关闭连接
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);
		// 取消定时器
		self->_deadline.cancel();
	});
}

void HttpConnection::HandleRequest()
{
	std::cout << "start Handle Request()..." << std::endl;
	std::cout << "[HandleRequest] method = " << _request.method_string()
		<< ", target = " << _request.target()
		<< ", this = " << this << std::endl;
	_response.version(_request.version());
	_response.keep_alive(false);
	if (_request.method() == http::verb::get) {
		// 调用LogicSystem处理get请求
		ParseGetParams();
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		// 处理发生错误
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		// 处理正常，返回成功
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}

	if (_request.method() == http::verb::post) {
		// 调用LogicSystem处理Post请求
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		// 处理发生错误
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		// 处理正常，返回成功
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}

void HttpConnection::ParseGetParams()
{
	auto uri = _request.target();

	auto query_pos = uri.find('?');
	// no params
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_str = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	// name=你好&email=123@qq.com
	size_t param_pos = 0;
	while ((param_pos = query_str.find('&')) != std::string::npos) {
        auto param = query_str.substr(0, param_pos);
		size_t eq_pos = param.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(param.substr(0, eq_pos));
            value = UrlDecode(param.substr(eq_pos + 1));
			_get_params[key] = value;
        }
		// erase "key=value&"
		query_str.erase(0, param_pos + 1);
	}

	if (!query_str.empty()) {
		size_t eq_pos = query_str.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_str.substr(0, eq_pos));
			value = UrlDecode(query_str.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}



}
