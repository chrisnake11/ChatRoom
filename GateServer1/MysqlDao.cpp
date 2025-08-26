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

bool MysqlDao::existUserByName(const std::string& username)
{
    auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });

    try {
        if(conn == nullptr) {
            _pool->returnConnection(std::move(conn));
            return false;
		}
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("select 1 from user_info where username = ?")
        );
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        // if result set is empty, return false
        if (res->next()) {
            return true;
        }
    }
    catch (std::exception& e) {
		std::cerr << "exception: " << e.what() << std::endl;
    }

    return false;
}

bool MysqlDao::existUserByEmail(const std::string& email)
{
    auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });

    try {
        if (conn == nullptr) {
            return false;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("select 1 from user_info where email = ?")
        );
        pstmt->setString(1, email);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            return true;
        }
    }
    catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }
    return false;
}

int MysqlDao::updateUserId()
{
    std::unique_ptr<SqlConnection> conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        if (conn == nullptr) {
			std::cerr << "get mysql connection is nullptr" << std::endl;
            return -1;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("UPDATE user_id SET id=(id+1)")
		);
        pstmt->executeUpdate();

		// 更新成功，返回更新后的用户ID
        return getMaxUserId();
    }
    catch (std::exception& e) {
		std::cout << "update user id exception: " << e.what() << std::endl;
    }
    return -1;
}

int MysqlDao::getMaxUserId()
{
    std::unique_ptr<SqlConnection> conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        if (conn == nullptr) {
            std::cerr << "get mysql connection is nullptr" << std::endl;
            return -1;
        }
        std::unique_ptr<sql::Statement> stmt(conn->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT id FROM user_id"));
        if (res->next()) {
            int id = res->getInt("id");
            return id;
        }
    }
    catch (std::exception& e) {
        std::cout << "update user id exception: " << e.what() << std::endl;
    }
    return -1;
}

int MysqlDao::createBaseUserInfo(const BaseUserInfo& user_info)
{
	auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
		});
    try {
        if (conn == nullptr) {
            return -1;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("INSERT INTO user_info(uid, username, email, passwd) VALUES(?,?,?,?)")
		);
        pstmt->setInt(1, user_info.uid);
        pstmt->setString(2, user_info.username);
        pstmt->setString(3, user_info.email);
        pstmt->setString(4, user_info.passwd);
        int ret = pstmt->executeUpdate();
        if (ret == 1) {
            return static_cast<int>(pstmt->getUpdateCount());
        }
    }
    catch(std::exception& e) {
        std::cout << "create base user info exception: " << e.what() << std::endl;
	}
    return -1;
}

int MysqlDao::createUserInfo(const UserInfo& user_info)
{
    auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        if (conn == nullptr) {
            return -1;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("INSERT INTO user_info(username, email, nickname, passwd, avatar, gender, address, phone, birthday, psersonal_signature, online_status, last_login, register_time) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)")
        );
		pstmt->setString(1, user_info.username);
		pstmt->setString(2, user_info.email);
		pstmt->setString(3, user_info.nickname);
		pstmt->setString(4, user_info.passwd);
		pstmt->setString(5, user_info.avatar);
        pstmt->setInt(6, user_info.gender);
        pstmt->setString(6, user_info.address);
		pstmt->setString(7, user_info.phone);
		pstmt->setString(8, user_info.birthday);
		pstmt->setString(9, user_info.sign);
        pstmt->setInt(10, user_info.online_status);
		pstmt->setDateTime(11, user_info.last_login);
		pstmt->setDateTime(12, user_info.register_time);
        int ret = pstmt->executeUpdate();
        if (ret == 1) {
            return static_cast<int>(pstmt->getUpdateCount());
        }
    }
    catch (std::exception& e) {
        std::cout << "create base user info exception: " << e.what() << std::endl;
    }
    return -1;
}

std::vector<UserInfo> MysqlDao::getUsersByNameAndPasswd(const std::string& name, const std::string passwd)
{
	std::vector<UserInfo> user_list;
	auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
		});
    try {
        if (conn == nullptr) {
			return user_list;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("SELECT * FROM user_info WHERE username = ? AND passwd = ?"));
		pstmt->setString(1, name);
		pstmt->setString(2, passwd);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            UserInfo user_info;
            user_info.uid = res->getInt("uid");
            user_info.username = res->getString("username");
            user_info.email = res->getString("email");
            user_info.passwd = res->getString("passwd");
            user_info.nickname = res->getString("nickname");
            user_info.phone = res->getString("phone");
            user_info.address = res->getString("address");
            user_info.avatar = res->getString("avatar");
            user_info.gender = res->getInt("gender");
			user_info.birthday = res->getString("birthday");
			user_info.sign = res->getString("personal_signature");
			user_info.online_status = res->getInt("online_status");
			user_info.last_login = res->getString("last_login");
			user_info.register_time = res->getString("register_time");
			user_list.push_back(user_info);
        }

    }
    catch (std::exception& e) {
		std::cout << "get user by name and passwd exception: " << e.what() << std::endl;
    }
    return user_list;
}

std::vector<UserInfo> MysqlDao::getUserByName(const std::string& name)
{
    std::vector<UserInfo> user_list;
    auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        if (conn == nullptr) {
            return user_list;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("SELECT * FROM user_info WHERE username = ?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            UserInfo user_info;
            user_info.uid = res->getInt("uid");
            user_info.username = res->getString("username");
            user_info.email = res->getString("email");
            user_info.passwd = res->getString("passwd");
            user_info.nickname = res->getString("nickname");
            user_info.phone = res->getString("phone");
            user_info.address = res->getString("address");
            user_info.avatar = res->getString("avatar");
            user_info.gender = res->getInt("gender");
            user_info.birthday = res->getString("birthday");
            user_info.sign = res->getString("personal_signature");
            user_info.online_status = res->getInt("online_status");
            user_info.last_login = res->getString("last_login");
            user_info.register_time = res->getString("register_time");
            user_list.push_back(user_info);
        }

    }
    catch (std::exception& e) {
        std::cout << "get user by name and passwd exception: " << e.what() << std::endl;
    }
    return user_list;
}

std::vector<UserInfo> MysqlDao::getUserById(int uid)
{
    std::vector<UserInfo> user_list;
    auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        if (conn == nullptr) {
            return user_list;
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("SELECT * FROM user_info WHERE uid=?"));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next()) {
            UserInfo user_info;
            user_info.uid = res->getInt("uid");
            user_info.username = res->getString("username");
            user_info.email = res->getString("email");
            user_info.passwd = res->getString("passwd");
            user_info.nickname = res->getString("nickname");
            user_info.phone = res->getString("phone");
            user_info.address = res->getString("address");
            user_info.avatar = res->getString("avatar");
            user_info.gender = res->getInt("gender");
            user_info.birthday = res->getString("birthday");
            user_info.sign = res->getString("personal_signature");
            user_info.online_status = res->getInt("online_status");
            user_info.last_login = res->getString("last_login");
            user_info.register_time = res->getString("register_time");
            user_list.push_back(user_info);
        }
    }
    catch (std::exception& e) {
        std::cout << "get user by name and passwd exception: " << e.what() << std::endl;
    }
    return user_list;   
}

int MysqlDao::updateUserPasswdByEmail(const std::string& email, const std::string& passwd)
{
	auto conn = _pool->getConnection();
    const Defer defer([this, &conn]() {
        _pool->returnConnection(std::move(conn));
        });
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_con->prepareStatement("UPDATE user_info SET passwd=? WHERE email=?"));
        pstmt->setString(1, passwd);
		pstmt->setString(2, email);
        int ret = pstmt->executeUpdate();
        if (ret == 1) {
            return static_cast<int>(pstmt->getUpdateCount());
		}
    }
    catch(std::exception& e) {
        std::cout << "update user passwd by email exception: " << e.what() << std::endl;
    }
    return -1;
}

bool MysqlDao::checkNameAndPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info) {
    auto connect = _pool->getConnection();
    Defer defer([this, &connect] {
        _pool->returnConnection(std::move(connect));
        });

    try {
        if (connect == nullptr) {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            connect->_con->prepareStatement("select * from user_info where username = ? and passwd = ?")
        );
        pstmt->setString(1, name);
        pstmt->setString(2, passwd);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        // if result set is empty, return false
        if (!res->next()) {
            std::cout << "password is not matched" << std::endl;
            return false;
        }
        user_info.uid = res->getInt("uid");
        user_info.username = res->getString("username");
        user_info.email = res->getString("email");
        user_info.passwd = res->getString("passwd");
        user_info.nickname = res->getString("nickname");
        user_info.phone = res->getString("phone");
        user_info.address = res->getString("address");
        user_info.avatar = res->getString("avatar");
        user_info.gender = res->getInt("gender");
        user_info.birthday = res->getString("birthday");
        user_info.sign = res->getString("personal_signature");
        user_info.online_status = res->getInt("online_status");
        user_info.last_login = res->getString("last_login");
        user_info.register_time = res->getString("register_time");
        return true;
    }

    catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}