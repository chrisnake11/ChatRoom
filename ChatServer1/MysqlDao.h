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


class SqlConnection {
public:
    SqlConnection(sql::Connection* con, int64_t last_time) :_con(con), _last_time(last_time) {}
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_time;
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


// 封装与MySQL数据库的操作
class MysqlDao {
public:
    MysqlDao();
    ~MysqlDao();

	std::unique_ptr<UserInfo> getUserInfo(int uid);

    // 更新用户登录状态和最后登录时间，如果登陆时间为空，则不更新时间。
    int updateLoginStatus(int uid, int status, const std::string& last_login);

    // 获取用户所有的聊天消息，以数组的形式返回
    std::unique_ptr<std::vector<MessageItem>> getMessageList(int uid);
    // 获取联系人列表，以数组的形式返回
    std::unique_ptr<std::vector<ContactItem>> getContactList(int uid);

private:
    std::unique_ptr<MysqlPool> _pool;
};
