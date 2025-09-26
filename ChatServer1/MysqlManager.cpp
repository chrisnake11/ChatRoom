#include "MysqlManager.h"
#include <exception>

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

MysqlManager::MysqlManager()
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

MysqlManager::~MysqlManager()
{
    _pool->close();
}

std::unique_ptr<UserInfo> MysqlManager::getUserInfo(int uid)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        return _dao.getUserInfo(uid, conn->_con.get());
    }
    catch (std::exception& e) {
        std::cerr << "get user info failed: " << e.what() << std::endl;
        return nullptr;
    }
}

int MysqlManager::updateLoginStatus(int uid, int status, const std::string& last_login)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return -1;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
	    int res = _dao.updateLoginStatus(uid, status, last_login, conn->_con.get());
        if (res < 1) {
            std::cerr << "update login status failed" << std::endl;
        }
        return res;
    }
    catch (std::exception& e) {
        std::cerr << "update login status failed: " << e.what() << std::endl;
        return -1;
    }
}

std::unique_ptr<std::vector<MessageInfo>> MysqlManager::getMessageList(int uid)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
	    return _dao.getMessageList(uid, conn->_con.get());
    }
    catch (std::exception& e) {
        std::cerr << "get message list failed: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<std::vector<ContactInfo>> MysqlManager::getContactList(int uid)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        return _dao.getContactList(uid, conn->_con.get());
    }
    catch (std::exception& e) {
        std::cerr << "get contact list failed: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<std::vector<ChatMessageInfo>> MysqlManager::getChatMessageList(const int& uid, const int& friend_uid, const int& last_message_id)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        return _dao.getChatMessageList(uid, friend_uid, last_message_id, conn->_con.get());
    }
    catch (std::exception& e) {
        std::cerr << "get chat message list failed: " << e.what() << std::endl;
        return nullptr;
    }
}

int MysqlManager::insertChatMessage(ChatMessageInfo& message)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return -1;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });

    try {
        conn->_con->setAutoCommit(false); // 开启事务
        int message_id = -1;

        // 1. 插入消息
        if (_dao.insertChatMessage(message, conn->_con.get()) <= 0) throw std::runtime_error("insert failed");

        // 2. 获取最新消息 ID
        message_id = _dao.getLastMessageId(message.sender_id, message.receiver_id, conn->_con.get());
        if (message_id <= 0) throw std::runtime_error("get last message_id failed");

        // 3. 更新 friend_relationship
        if (_dao.updateFriendRelationshipMessage(message_id, message.sender_id, message.receiver_id, conn->_con.get()) <= 0)
            throw std::runtime_error("update friend_relationship failed");

        // 提交事务
        conn->_con->commit();
        conn->_con->setAutoCommit(true);

        return message_id;
    }
    catch (std::exception& e) {
        try {
            conn->_con->rollback();
        }
        catch (const std::exception& rollback_e) {
            std::cerr << "rollback failed: " << rollback_e.what() << std::endl;
        }
        try {
            conn->_con->setAutoCommit(true);
        }
        catch (...) {}
        std::cerr << "insertChatMessage failed: " << e.what() << std::endl;
        return -1;
    }
}

std::unique_ptr<FriendRelationship> MysqlManager::getFriendRelationship(std::unique_ptr<FriendRelationship>fr)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        return _dao.getFriendRelationship(std::move(fr), conn->_con.get());
    }
    catch (std::exception& e) {
        std::cerr << "get friend relationship failed: " << e.what() << std::endl;
        return nullptr;
    }
}

int MysqlManager::updateFriendRelationship(std::unique_ptr<FriendRelationship>fr)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return -1;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        return _dao.updateFriendRelationship(std::move(fr), conn->_con.get());
    }
    catch (std::exception& e) {
        std::cerr << "update friend relationship failed: " << e.what() << std::endl;
        return -1;
    }
}

std::unique_ptr<std::vector<SearchFriendInfo>> MysqlManager::searchFriendList(const std::string& friend_name)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return nullptr;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        return _dao.searchFriendList(friend_name, conn->_con.get());
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQL Error in searchFriendList: " << e.what()
            << " (MySQL error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return nullptr;  // 返回错误码
    }
    catch (std::exception& e) {
        std::cerr << "search friend list by friend_name failed: " << e.what() << std::endl;
        return nullptr;
    }
}

int MysqlManager::addFriendRequest(const int& uid, const int& friend_uid)
{
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return -1;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        bool isExist = _dao.existFriendRequest(uid, friend_uid, conn->_con.get());
		// 已存在，返回0
        if (isExist) {
            throw std::runtime_error("friend request is exist");
		}
		return _dao.insertFriendRequest(uid, friend_uid, conn->_con.get());
    }
    // 失败返回-1
    catch (sql::SQLException& e) {
        std::cerr << "SQL Error in addFriendRequest: " << e.what()
            << " (MySQL error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return -1;  // 返回错误码
    }
    catch (std::exception& e) {
        std::cerr << "search friend list by friend_name failed: " << e.what() << std::endl;
        return -1;
    }
}

std::unique_ptr<std::vector<AddFriendInfo>> MysqlManager::getFriendReuqestList(const int& uid)
{
    std::unique_ptr<std::vector<AddFriendInfo>> addFriendList = std::make_unique<std::vector<AddFriendInfo>>();
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return addFriendList;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        conn->_con->setAutoCommit(false); // 开启事务

        // 查询自己发送的请求
        std::unique_ptr<std::vector<AddFriendInfo>> requestList = _dao.getFriendRequestList(uid, conn->_con.get());
        // 查询收到的请求
        std::unique_ptr<std::vector<AddFriendInfo>> responseList = _dao.getFriendResponseList(uid, conn->_con.get());

        if (!requestList && !responseList) {
            return addFriendList;
        }

        // 预留足够空间以提高性能
        size_t total_size = (requestList ? requestList->size() : 0) + (responseList ? responseList->size() : 0);
        addFriendList->reserve(total_size);

        // 合并结果到addFriendList
        if (requestList) {
            addFriendList->insert(addFriendList->end(), std::make_move_iterator(requestList->begin()), std::make_move_iterator(requestList->end()));
        }
        if (responseList) {
            addFriendList->insert(addFriendList->end(), std::make_move_iterator(responseList->begin()), std::make_move_iterator(responseList->end()));

        }
        // 根据日期排序 降序排序
        std::sort(addFriendList->begin(), addFriendList->end(), [](const AddFriendInfo& lhs, const AddFriendInfo& rhs) {
            return lhs.request_time > rhs.request_time;
            });

        // 提交事务
        conn->_con->commit();
        conn->_con->setAutoCommit(true);


        return addFriendList;
    }
    // 失败
    catch (sql::SQLException& e) {
        conn->_con->rollback();
        std::cerr << "SQL Error in getFriendReuqestList: " << e.what()
            << " (MySQL error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return addFriendList;  // 返回错误码
    }
    catch (std::exception& e) {
        std::cerr << "search friend list by friend_name failed: " << e.what() << std::endl;
        return addFriendList;
    }
}

int MysqlManager::acceptFriendRequest(const int& sender_id, const int& receiver_id)
{
    int res = -1;
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return res;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        conn->_con->setAutoCommit(false);
        // request是否存在
        if (!_dao.existFriendRequest(sender_id, receiver_id, conn->_con.get())) {
            throw std::runtime_error("accept friend request not exist!");
        }

        // 同意好友请求
        int res = _dao.acceptFriendRequest(sender_id, receiver_id, conn->_con.get());
        if (res >= 0) {
            // 添加信息到好友关系列表
            _dao.addFriendRelationship(sender_id, receiver_id, conn->_con.get());
        }
        conn->_con->commit();
        conn->_con->setAutoCommit(true);
        return res;
    }
    catch (sql::SQLException& e) {
        conn->_con->rollback();
        std::cerr << "SQL Error in acceptFriendRequest: " << e.what()
            << " (MySQL error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return -1;  // 返回错误码
    }
    catch (const std::exception& e) {
        std::cerr << "update accept friend request failed: " << e.what() << std::endl;
        return -1;
    }

}

int MysqlManager::rejectFriendRequest(const int& sender_id, const int& receiver_id)
{
    int res = -1;
    auto conn = _pool->getConnection();
    if (!conn) {
        std::cerr << "get mysql connection failed" << std::endl;
        return res;
    }
    Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        conn->_con->setAutoCommit(false);
        // request是否存在
        if (!_dao.existFriendRequest(sender_id, receiver_id, conn->_con.get())) {
            throw std::runtime_error("accept friend request not exist!");
        }
        
        // update reject 状态信息
        int res = _dao.rejectFriendRequest(sender_id, receiver_id, conn->_con.get());
        
        conn->_con->commit();
        conn->_con->setAutoCommit(true);

        return res;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQL Error in rejectFriendRequest: " << e.what()
            << " (MySQL error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return res;  // 返回错误码
    }
    catch (const std::exception& e) {
        std::cerr << "update accept friend request failed: " << e.what() << std::endl;
        return res;
    }

}

