#pragma once
#include "Const.h"
#include "Singleton.h"
#include <map>
#include <functional>
#include <iostream>
#include "HttpConnection.h"

class HttpConnection;

typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	LogicSystem(const LogicSystem&) = delete;
    LogicSystem& operator=(const LogicSystem&) = delete;
	~LogicSystem() {
		std::cout << "LogicSystem Destructor" << std::endl;
	}

	// Handlers
	bool HandleGet(std::string url, std::shared_ptr<HttpConnection> http_conn);
	bool HandlePost(std::string url, std::shared_ptr<HttpConnection> http_conn);

private:
	void RegisterPost(std::string, HttpHandler handler);
	void RegisterGet(std::string, HttpHandler handler);
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};

