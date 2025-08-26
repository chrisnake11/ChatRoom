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

    bool existUserByName(const std::string& username);
    bool existUserByEmail(const std::string& email);
    
	// 更新user_id表中的最大用户ID，返回更新后的用户ID
	int updateUserId();
	// 获取user_id表中的最大用户ID
	int getMaxUserId();

	// 创建用户信息到user_info表，返回主键id
	int createBaseUserInfo(const BaseUserInfo& user_info);
    int createUserInfo(const UserInfo& user_info);

    // 获取查询用户
    std::vector<UserInfo> getUsersByNameAndPasswd(const std::string& name, const std::string passwd);
	std::vector<UserInfo> getUserByName(const std::string& name);
	std::vector<UserInfo> getUserById(int uid);
    
	// 通过邮箱更新用户密码,返回受影响的行数
	int updateUserPasswdByEmail(const std::string& email, const std::string& passwd);

    bool checkNameAndPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info);

private:
    std::unique_ptr<MysqlPool> _pool;
};
