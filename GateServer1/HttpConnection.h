#pragma once

#include "Const.h"

class LogicSystem;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
    friend class LogicSystem;
    HttpConnection(net::io_context& io_context);
    void Start();
    tcp::socket& GetSocket();

private:
    void CheckDeadline();
    void WriteResponse();
    // ����HTTP���󣬲�����
    void HandleRequest();
    void ParseGetParams();

    tcp::socket _socket;
    //GateServer* _gate_server;
    beast::flat_buffer _buffer{4096};
    http::request<http::dynamic_body> _request;
    http::response<http::dynamic_body> _response;
    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;

    // {} ���г�ʼ�����
    net::steady_timer _deadline;
};

