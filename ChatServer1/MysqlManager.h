#pragma once
#include "Const.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <memory>
#include "ConfigManager.h"
#include "Data.h"
#include <vector>
#include "MysqlDao.h"

class SqlConnection {
public:
    SqlConnection(sql::Connection* con, int64_t last_time) :_con(con), _last_time(last_time) {}
    // 指针由智能指针管理，外部程序只使用裸指针销，不delete指针。
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_time;

    SqlConnection(const SqlConnection&) = delete;
    SqlConnection& operator=(const SqlConnection&) = delete;
    SqlConnection(SqlConnection&&) = default;
    SqlConnection& operator=(SqlConnection&&) = default;
};

class MysqlPool
{
public:
    MysqlPool(const std::string& url, const std::string& user, const std::string& passwd,
        const std::string& schema, std::size_t pool_size);
    std::unique_ptr<SqlConnection> getConnection();
    void returnConnection(std::unique_ptr<SqlConnection> conn);
    void checkConnection();
    void close();
    ~MysqlPool();

private:
    std::string _url;
    std::string _user;
    std::string _passwd;
    std::string _schema;
    std::size_t _pool_size;
    std::queue<std::unique_ptr<SqlConnection>> _pool;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::atomic<bool> _b_stop;

    std::thread _check_thread;
};



// 单例类进一步封装MySQLDao类
class MysqlManager : public Singleton<MysqlManager>
{
	friend class Singleton<MysqlManager>;
public:
	~MysqlManager();

	std::unique_ptr<UserInfo> getUserInfo(int uid);
	int updateLoginStatus(int uid, int status, const std::string& last_login);
	std::unique_ptr<std::vector<MessageInfo>> getMessageList(int uid);
	std::unique_ptr<std::vector<ContactInfo>> getContactList(int uid);
	std::unique_ptr<std::vector<ChatMessageInfo>> getChatMessageList(const int& uid, const int& friend_uid, const int& last_message_id);

    // 插入聊天消息，更新好友状态，返回最新的消息id
	int insertChatMessage(ChatMessageInfo& message);

	std::unique_ptr<FriendRelationship> getFriendRelationship(std::unique_ptr<FriendRelationship>);
	int updateFriendRelationship(std::unique_ptr<FriendRelationship>);

    // 模糊搜索好友的username和nickname
	std::unique_ptr<std::vector<SearchFriendInfo>> searchFriendList(const std::string& friend_name);

    // 添加好友请求
	int addFriendRequest(const int& uid, const int& friend_uid);

    // 查询添加好友请求列表 
    std::unique_ptr<std::vector<AddFriendInfo>> getFriendReuqestList(const int& uid);

    // 同意好友请求
    int acceptFriendRequest(const int& sender_id, const int& receiver_id);
    // 更新好友请求
    int rejectFriendRequest(const int& sender_id, const int& receiver_id);
private:
	MysqlManager();
	MysqlDao _dao;
	std::unique_ptr<MysqlPool> _pool;
};

