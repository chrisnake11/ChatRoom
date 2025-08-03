#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisManager.h"
#include "MysqlManager.h"
#include "StatusGrpcClient.h"

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> http_conn)
{
	// ��ָ�� path ��GetHandler
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}
	// ����GetHandler
	_get_handlers[path](http_conn);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> http_conn)
{
	// ��ָ�� path ��PostHandler
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}
	// ����PostHandler
	_post_handlers[path](http_conn);
	return true;
}

void LogicSystem::RegisterGet(std::string url, HttpHandler handler)
{
	// ���һ�� GetHandler
	_get_handlers.insert(std::make_pair(url, handler));
}

void LogicSystem::RegisterPost(std::string url, HttpHandler handler)
{
	// ���һ�� PostHandler
	_post_handlers.insert(std::make_pair(url, handler));
}

LogicSystem::LogicSystem() {
	// ע��һ�� GetHandler
	RegisterGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test request" << std::endl;
			int i = 0;
			for (auto& pair : connection->_get_params) {
				i++;
				beast::ostream(connection->_response.body()) << "param [" << i << "] key is " << pair.first << std::endl;
				beast::ostream(connection->_response.body()) << "param [" << i << "] value is " << pair.second << std::endl;
			}
		});

	RegisterPost("/get_varify_code", [](std::shared_ptr<HttpConnection> connection) {
		// ��ȡ������
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		std::cout << "request_body is: " << body_str << std::endl;

		// ������Ӧͷ
		connection->_response.set(http::field::content_type, "text/plain");

		Json::Value root;
		Json::Reader reader;
		Json::Value response_root;
		// ����������Json����
		bool parse_success = reader.parse(body_str, root);
		if (!parse_success) {
			std::cout << "Failed to parse json" << std::endl;
			response_root["error"] = ErrorCodes::ERROR_JSON_PARSE_FAILED;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}


		// ��ȡ������Json����
		auto email = root["email"].asString();
		std::cout << "email: " << email << std::endl;

		// ʹ��Grpc������֤��������������
		GetVarifyRsp response = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
		root["error"] = response.error();

		// ��������ظ��ͻ���
		response_root["error"] = response.error();
		response_root["code"] = response.code();
        std::string response_str = response_root.toStyledString();
        beast::ostream(connection->_response.body()) << response_str;
		return true;
		
		});

	RegisterPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		// ��ȡ������
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		std::cout << "request_body is: " << body_str << std::endl;

		// ������Ӧͷ
		connection->_response.set(http::field::content_type, "text/plain");

		Json::Value root;
		Json::Reader reader;
		Json::Value response_root;
		// ����������Json����
		bool parse_success = reader.parse(body_str, root);
		// ����ʧ��
		if (!parse_success) {
			std::cout << "Failed to parse json" << std::endl;
			response_root["error"] = ErrorCodes::ERROR_JSON_PARSE_FAILED;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}

		// ��ѯRedis�е���֤��
		std::string varify_code;
		bool b_get_varify_code = RedisManager::GetInstance()->Get(CODEPREFIX+root["email"].asString(), varify_code);
		std::cout << "get varify code: " << CODEPREFIX + root["email"].asString() << " " << varify_code << std::endl;
		// ��ѯʧ��
		if (!b_get_varify_code) {
			response_root["error"] = ErrorCodes::ERROR_VARIFY_EXPIRED;
			std::cout << " varify code expired" << std::endl;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}

		if (varify_code != root["code"].asString()) {
			response_root["error"] = ErrorCodes::ERROR_VARIFY_WRONG;
			std::cout << "varify code wrong" << std::endl;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}
		
		// MySQL�����û�
		int uid = MysqlManager::GetInstance()->RegisterUser(root["name"].asString(), root["email"].asString(), root["passwd"].asString());

		// �û��Ѵ���
		if (uid == 0 || uid == -1) {
			std::cout << "user or email exist" << std::endl;
			response_root["error"] = ErrorCodes::ERROR_USER_EXIST;
			std::string jsonstr = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

			
		// �ɹ���ɾ����֤��
		RedisManager::GetInstance()->Del(CODEPREFIX+root["email"].asString());

		// ������Ӧ���
		response_root["error"] = ErrorCodes::SUCCESS;
		response_root["uid"] = uid;
		response_root["name"] = root["name"];
        response_root["email"] = root["email"];
        response_root["passwd"] = root["passwd"];
        response_root["confirm"] = root["confirm"];
		response_root["code"] = root["code"];
		std::string json_str = response_root.toStyledString();
		beast::ostream(connection->_response.body()) << json_str;
		return true;
		});

	RegisterPost("/user_reset", [](std::shared_ptr<HttpConnection> connection) {
		// ��ȡ������
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		std::cout << "request_body is: " << body_str << std::endl;

		// ������Ӧͷ
		connection->_response.set(http::field::content_type, "text/plain");

		Json::Value root;
		Json::Reader reader;
		Json::Value response_root;
		// ����������Json����
		bool parse_success = reader.parse(body_str, root);
		// ����ʧ��
		if (!parse_success) {
			std::cout << "Failed to parse json" << std::endl;
			response_root["error"] = ErrorCodes::ERROR_JSON_PARSE_FAILED;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}

		// ��ѯRedis�е���֤��
		std::string varify_code;
		bool b_get_varify_code = RedisManager::GetInstance()->Get(CODEPREFIX + root["email"].asString(), varify_code);
		std::cout << "get varify code: " << CODEPREFIX + root["email"].asString() << " " << varify_code << std::endl;
		// ��ѯʧ��
		if (!b_get_varify_code) {
			response_root["error"] = ErrorCodes::ERROR_VARIFY_EXPIRED;
			std::cout << " varify code expired" << std::endl;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}

		if (varify_code != root["code"].asString()) {
			response_root["error"] = ErrorCodes::ERROR_VARIFY_WRONG;
			std::cout << "varify code wrong" << std::endl;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}

		// MySQL search email and reset password
		int uid = MysqlManager::GetInstance()->ResetUser(root["email"].asString(), root["passwd"].asString());

		// email not exists
		if (uid == 0 || uid == -1) {
			std::cout << "reset failed, email not exist" << std::endl;
			response_root["error"] = ErrorCodes::ERROR_EMAIL_NOT_MATCH;
			std::string jsonstr = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		// �ɹ���ɾ����֤��
		RedisManager::GetInstance()->Del(CODEPREFIX+root["email"].asString());

		// ������Ӧ���
		response_root["error"] = ErrorCodes::SUCCESS;
		response_root["uid"] = uid;
		response_root["email"] = root["email"];
		response_root["passwd"] = root["passwd"];
		response_root["confirm"] = root["confirm"];
		response_root["code"] = root["code"];
		std::string json_str = response_root.toStyledString();
		beast::ostream(connection->_response.body()) << json_str;
		return true;
		});

	RegisterPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		// ��ȡ������
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		std::cout << "request_body is: " << body_str << std::endl;

		// ������Ӧͷ
		connection->_response.set(http::field::content_type, "text/plain");

		Json::Value root;
		Json::Reader reader;
		Json::Value response_root;

		// ����������Json����
		bool parse_success = reader.parse(body_str, root);
		// ����ʧ��
		if (!parse_success) {
			std::cout << "Failed to parse json" << std::endl;
			response_root["error"] = ErrorCodes::ERROR_JSON_PARSE_FAILED;
			std::string response_str = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << response_str;
			return true;
		}

		auto name = root["name"].asString();
		auto passwd = root["passwd"].asString();
		UserInfo user_info;
		
		// ��֤����
		bool passwd_valid = MysqlManager::GetInstance()->checkPasswd(name, passwd, user_info);
		if (!passwd_valid) {
			std::cout << "passwd wrong" << std::endl;
			root["error"] = ErrorCodes::ERROR_PASSWORD_WRONG;
			std::string jsonstr = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// ��ȡ�û�״̬
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(user_info.uid);
		// ��ӡ��־
		std::cout << "get status reply is: " 
			<< reply.host() << ", " 
			<< reply.port() << ", " 
			<< reply.token() << ", " 
			<< reply.error() 
			<< std::endl;

		if (reply.error()) {
            std::cout << "get status failed. error is: " << reply.error() << std::endl;
			root["error"] = ErrorCodes::ERROR_RPC_FAILED;
			std::string jsonstr = response_root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
            return true;
		}
		

		// ������Ӧ���
		std::cout << "success to load userinfo. uid is: " << user_info.uid << std::endl;
		response_root["error"] = ErrorCodes::SUCCESS;
        response_root["name"] = name;
		response_root["uid"] = user_info.uid;
		response_root["token"] = reply.token();
		response_root["host"] = reply.host();
		response_root["port"] = reply.port();
		std::string json_str = response_root.toStyledString();
		beast::ostream(connection->_response.body()) << json_str;
		return true;
		});

}
