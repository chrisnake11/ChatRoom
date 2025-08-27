#include "MysqlDao.h"
#include <chrono>
#include "MysqlManager.h"

MysqlPool::MysqlPool(const std::string& url, const std::string& user, const std::string& passwd,
    const std::string& schema, std::size_t pool_size)
    : _url(url), _user(user), _passwd(passwd), _schema(schema), _pool_size(pool_size) {
    try {
        for (std::size_t i = 0; i < _pool_size; ++i) {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            auto* con = driver->connect(url, user, passwd);
            con->setSchema(schema);
            auto current_time = std::chrono::system_clock::now().time_since_epoch();
            long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
            _pool.push(std::make_unique<SqlConnection>(con, timestamp));
        }

        _check_thread = std::thread([this]() {
            while (!_b_stop) {
                checkConnection();
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            });
        //  启动检查线程(守护线程)
        _check_thread.detach();
    }
    catch (sql::SQLException& e) {
        std::cout << "SQL Exception is " << e.what() << std::endl;
    }
}

MysqlPool::~MysqlPool() {
    std::unique_lock<std::mutex> lock(_mutex);
    // 为什么只需要清空队列，而不需要释放连接呢？
    while (!_pool.empty()) {
        _pool.pop();
    }
}

void MysqlPool::checkConnection() {
    std::lock_guard<std::mutex> lock(_mutex);
    int pool_size = _pool.size();
    auto current_time = std::chrono::system_clock::now().time_since_epoch();
    long long time_now = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
    for (std::size_t i = 0; i < pool_size; ++i) {
        std::unique_ptr<SqlConnection> con = std::move(_pool.front());
        _pool.pop();
        Defer defer([this, &con]() {
            _pool.push(std::move(con));
            });

        // 5秒内检查过，则跳过
        if (time_now - con->_last_time < 5) {
            continue;
        }

        // 检查连接是否可用
        try {
            // 创建Statement
            std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
            stmt->executeQuery("select 1");
            con->_last_time = time_now;
            std::cout << "execute timer alive query, cur is " << time_now << std::endl;
        }
        catch (sql::SQLException& e) {
            // 连接异常, 重新创建连接，并替换原连接
            std::cout << "Error keeping connection alive: " << e.what() << std::endl;
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            auto* new_con = driver->connect(_url, _user, _passwd);
            new_con->setSchema(_schema);
            con->_con.reset(new_con);
            con->_last_time = time_now;
        }
    }
}

void MysqlPool::returnConnection(std::unique_ptr<SqlConnection> conn)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _pool.push(std::move(conn));
    _cond.notify_one();
}

void MysqlPool::close() {
    _b_stop = true;
    // 唤醒所有等待的线程
    _cond.notify_all();
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    // 当Pool要关闭 或者 队列有连接时，继续执行。
    _cond.wait(lock, [this]() {
        return _b_stop ? true : !_pool.empty();
        });

    // 如果Pool关闭，返回nullptr
    if (_b_stop) {
        return nullptr;
    }

    // 获取队列头部的连接
    std::unique_ptr<SqlConnection> con(std::move(_pool.front()));
    _pool.pop();
    return con;
}

MysqlDao::MysqlDao()
{
    // 获取数据库连接信息，初始化连接池
    auto& config = ConfigManager::GetInstance();
    const auto& host = config["MySQL"]["Host"];
    const auto& user = config["MySQL"]["User"];
    const auto& passwd = config["MySQL"]["Password"];
    const auto& schema = config["MySQL"]["Schema"];
    const auto& port = config["MySQL"]["Port"];

    _pool.reset(new MysqlPool(host + ":" + port, user, passwd, schema, 5));
}

MysqlDao::~MysqlDao()
{
    _pool->close();
}

std::unique_ptr<UserInfo> MysqlDao::getUserInfo(int uid)
{
	// 从数据库查询用户基本信息
    std::unique_ptr<UserInfo> user_info = nullptr;
    std::unique_ptr<SqlConnection> conn = _pool->getConnection();
    if (conn == nullptr) {
        std::cout << "get mysql connection failed." << std::endl;
        return user_info;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("SELECT * from user_info where uid = ?"));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            user_info = std::make_unique<UserInfo>();
            user_info->uid = res->getInt("uid");
            user_info->username = res->getString("username");
            user_info->email = res->getString("email");
            user_info->passwd = res->getString("passwd");
            user_info->nickname = res->getString("nickname");
            user_info->phone = res->getString("phone");
            user_info->address = res->getString("address");
            user_info->avatar = res->getString("avatar");
            user_info->gender = res->getInt("gender");
            user_info->birthday = res->getString("birthday");
            user_info->sign = res->getString("personal_signature");
            user_info->online_status = res->getInt("online_status");
            user_info->last_login = res->getString("last_login");
            user_info->register_time = res->getString("register_time");
        }
        return user_info;
    }
    catch(sql::SQLException& e) {
        std::cout << "SQL Exception is " << e.what() << std::endl;
        return user_info;
	}
}

int MysqlDao::updateLoginStatus(int uid, int status, const std::string& last_login) {
    auto conn = _pool->getConnection();
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    if (conn == nullptr) {
        std::cout << "get mysql connection failed." << std::endl;
        return -1;
    }
    try {
        if (!last_login.empty()) {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->_con->prepareStatement("UPDATE user_info SET online_status = ?, last_login = ? WHERE uid = ?"));
            pstmt->setInt(1, status);
            pstmt->setString(2, last_login);
            pstmt->setInt(3, uid);
            return pstmt->executeUpdate();
        }
        else {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                conn->_con->prepareStatement("UPDATE user_info SET online_status = ? WHERE uid = ?"));
            pstmt->setInt(1, status);
            pstmt->setInt(2, uid);
            return pstmt->executeUpdate();
        }
        
    }
    catch (std::exception& e) {
        std::cout << "SQL Exception is " << e.what() << std::endl;
        return -1;
    }
}

std::unique_ptr<std::vector<MessageItem>> MysqlDao::getMessageList(int uid) {
    auto conn = _pool->getConnection();
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });

    if (conn == nullptr) {
        std::cout << "get mysql connection failed." << std::endl;
        return nullptr;
    }

    try {
        std::string query_str = "SELECT fr.friend_id, fr.last_message_id, fr.unread_count, msg.content, msg.timestamp, ui.nickname, ui.avatar "
            "FROM friend_relationship fr LEFT JOIN message msg "
            "ON fr.last_message_id = msg.message_id "
            "LEFT JOIN user_info ui "
            "ON fr.friend_id = ui.id "
            "WHERE fr.user_id = ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement(query_str));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::unique_ptr<std::vector<MessageItem>> message_list = std::make_unique<std::vector<MessageItem>>();
        while (res->next()) {
            // 将结果添加到message_list中
            MessageItem message_item;
            message_item.uid = res->getInt("friend_id");
            message_item.nickname = res->getString("nickname");
            message_item.avatar = res->getString("avatar");
            message_item.message = res->getString("content");
            message_item.last_message_time = res->getString("timestamp");
            message_item.unread_count = res->getInt("unread_count");
            message_list->emplace_back(message_item);
            /*message_list->emplace_back(res->getInt("friend_id"), res->getString("nickname"), res->getString("avatar")
                , res->getString("message"), res->getString("timestamp"), res->getInt("unread_count"));*/
        }
        return std::move(message_list);
    }
    catch (std::exception& e) {
        std::cerr << "query message liss failed, SQL exception is: " << e.what() << std::endl;
        return nullptr;
    }
}