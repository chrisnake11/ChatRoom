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
    std::unique_ptr<SqlConnection> GetConnection();
    void ReturnConnection(std::unique_ptr<SqlConnection> conn);
    void CheckConnection();
    void Close();
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

    // 用户注册操作
    int RegisterUser(const std::string& name, const std::string& email, const std::string& passwd);

    int ResetUser(const std::string& email, const std::string& passwd);

    bool checkPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info);

private:
    std::unique_ptr<MysqlPool> _pool;
};
