#include "TcpManager.h"
#include <QAbstractSocket>
#include <QJsonDocument>
#include "UserManager.h"


TcpManager::TcpManager() : _host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0) {
	// 绑定连接事件
	QObject::connect(&_socket, &QTcpSocket::connected, [&] {
		qDebug() << "Success Connect to Server";
		emit sig_connect_success(true);
		});

	// 绑定读取事件
	QObject::connect(&_socket, &QTcpSocket::readyRead, [&] {
		_buffer.append(_socket.readAll());

		QDataStream stream(&_buffer, QIODevice::ReadOnly);
		stream.setVersion(QDataStream::Qt_6_0);

		// 循环读取数据
		forever{
			if (!_b_recv_pending) {
				// 读取header，数据长度不够
				if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) {
					return;
				}

				stream >> _message_id >> _message_len;

				_buffer = _buffer.mid(sizeof(quint16) * 2);

				qDebug() << "Message id: " << _message_id << ", Message len: " << _message_len;
			}

			// 数据体没读完
			if (_buffer.size() < _message_len) {
				_b_recv_pending = true;
				return;
			}


			// 读取消息体
			_b_recv_pending = false;
			QByteArray message_body = _buffer.mid(0, _message_len);
			qDebug() << "recv body is: " << message_body;

			// 处理数据包
			_buffer = _buffer.mid(_message_len);
			handleMsg(RequestID(_message_id), _message_len, message_body);
		}

		});

	// socket连接异常处理
	 QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
           Q_UNUSED(socketError)
           qDebug() << "Error:" << _socket.errorString();
		   emit sig_connect_success(false);
       });

	 // 处理连接断开
	 QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
		 qDebug() << "Disconnected from server.";
		 });

	 // 绑定发送信号来发送数据
	 QObject::connect(this, &TcpManager::sig_send_data, this, &TcpManager::slot_send_data);

	 initHandlers();
}

TcpManager::~TcpManager() {

}

void TcpManager::initHandlers() {
	// register response message's handler
	_handlers.insert(ID_CHAT_LOGIN_RSP, [this](RequestID id, int len, QByteArray data) {
		Q_UNUSED(len);
		qDebug() << "handle id is: " << id << "data is " << data;
		
		// parse json data		
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

		if (jsonDoc.isNull()) {
			qDebug() << "Failed to create JsonDocument";
			return;
		}

		QJsonObject jsonObj = jsonDoc.object();
		if (!jsonObj.contains("error")) {
			int err = ErrorCodes::ERROR_JSON_PARSE_FAILED;
			qDebug() << "Login Failed, error is json parse error" << err;
			emit sig_login_failed(err);
			return;
		}

		// check success signals
		int err = jsonObj["error"].toInt();
		if (err != ErrorCodes::SUCCESS) {
			qDebug() << "Login failed, err is " << err;
			emit sig_login_failed(err);
			return;
		}

		// create user and change user login status
		UserManager::getInstance()->setUid(jsonObj["uid"].toInt());
		UserManager::getInstance()->setName(jsonObj["name"].toString());
		UserManager::getInstance()->setToken(jsonObj["token"].toString());


		// change to chat dialog
		emit sig_switch_chat_dialog();
		});
}

void TcpManager::handleMsg(RequestID id, int len, QByteArray data) {
	auto it = _handlers.find(id);
	if (it == _handlers.end()) {
		qDebug() << "not found id " << id << " handler function";
		return;
	}
	it.value()(id, len, data);
}

void TcpManager::slot_tcp_connect(ServerInfo server_info) {
	qDebug() << "Connecting to chat server";
	_host = server_info.host;
	_port = static_cast<uint16_t>(server_info.port.toUInt());
	qDebug() << "Target ChatServer Host: " << _host << ", Port: " << _port;
	_socket.connectToHost(_host, _port);
}

void TcpManager::slot_send_data(RequestID reqId, QString data) {
	uint16_t id = reqId;

	QByteArray dataBytes = data.toUtf8();
	quint16 len = static_cast<quint16>(data.size());

	// create a write-only buffer to send data
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out.setByteOrder(QDataStream::BigEndian);
	out << id << len;
	block.append(dataBytes);

	// send data
	_socket.write(block);

}