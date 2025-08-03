#pragma once
#include "Const.h"
class MsgNode{
public:
	MsgNode(int max_len): _total_length(max_len), _cur_length(0) {
		_data = new char[_total_length + 1]();
		_data[_total_length] = '\0';
	}

	void clear() {
		memset(_data, 0, _total_length);
	}
	char* _data;
	int _total_length;
	int _cur_length;
};

class RecvNode : public MsgNode{
public:
	RecvNode(int total_length, int msg_id) : MsgNode(total_length), _msg_id(msg_id){
	
	}
	int _msg_id;
};

class SendNode : public MsgNode {
public:
	SendNode(const char* msg, int max_length, int msg_id):
		MsgNode(max_length + HEAD_TOTAL_LENGTH), _msg_id(msg_id){
		// 发送前将头部数据转化为大端序。
		short host_msg_id = boost::asio::detail::socket_ops::host_to_network_short(_msg_id);
		memcpy(_data, &host_msg_id, HEAD_ID_LENGTH);
		short host_max_length = boost::asio::detail::socket_ops::host_to_network_short(_total_length);
		memcpy(_data + HEAD_ID_LENGTH, &host_max_length, HEAD_DATA_LENGTH);
		memcpy(_data + HEAD_TOTAL_LENGTH, msg, max_length);
	}

	int _msg_id;
};