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
                CheckConnection();
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            });
        //  ��������߳�(�ػ��߳�)
        _check_thread.detach();
    }
    catch (sql::SQLException& e) {
        std::cout << "SQL Exception is " << e.what() << std::endl;
    }
}

MysqlPool::~MysqlPool() {
    std::unique_lock<std::mutex> lock(_mutex);
    // Ϊʲôֻ��Ҫ��ն��У�������Ҫ�ͷ������أ�
    while (!_pool.empty()) {
        _pool.pop();
    }
}

void MysqlPool::CheckConnection() {
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

        // 5���ڼ�����������
        if (time_now - con->_last_time < 5) {
            continue;
        }

        // ��������Ƿ����
        try {
            // ����Statement
            std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
            stmt->executeQuery("select 1");
            con->_last_time = time_now;
            std::cout << "execute timer alive query, cur is " << time_now << std::endl;
        }
        catch (sql::SQLException& e) {
            // �����쳣, ���´������ӣ����滻ԭ����
            std::cout << "Error keeping connection alive: " << e.what() << std::endl;
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            auto* new_con = driver->connect(_url, _user, _passwd);
            new_con->setSchema(_schema);
            con->_con.reset(new_con);
            con->_last_time = time_now;
        }
    }
}

void MysqlPool::ReturnConnection(std::unique_ptr<SqlConnection> conn)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _pool.push(std::move(conn));
    _cond.notify_one();
}

void MysqlPool::Close() {
    _b_stop = true;
    // �������еȴ����߳�
    _cond.notify_all();
}

std::unique_ptr<SqlConnection> MysqlPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    // ��PoolҪ�ر� ���� ����������ʱ������ִ�С�
    _cond.wait(lock, [this]() {
        return _b_stop ? true : !_pool.empty();
        });

    // ���Pool�رգ�����nullptr
    if (_b_stop) {
        return nullptr;
    }

    // ��ȡ����ͷ��������
    std::unique_ptr<SqlConnection> con(std::move(_pool.front()));
    _pool.pop();
    return con;
}

MysqlDao::MysqlDao()
{
    // ��ȡ���ݿ�������Ϣ����ʼ�����ӳ�
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
    _pool->Close();
}

int MysqlDao::RegisterUser(const std::string& name, const std::string& email, const std::string& passwd)
{
    auto conn = _pool->GetConnection();
    try {
        if (conn == nullptr) {
            _pool->ReturnConnection(std::move(conn));
            return -1;
        }

        const Defer defer([this, &conn]() {
            _pool->ReturnConnection(std::move(conn));
            });

        // ִ��MySQL����
        std::unique_ptr<sql::PreparedStatement> stmt(conn->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, passwd);
        stmt->execute();

        // ����statement��ѯ���
        std::unique_ptr<sql::Statement> stmtResult(conn->_con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmtResult->executeQuery("select @result AS result"));
        // ��ѯ����ɹ�
        if (rs->next()) {
            int result = rs->getInt("result");
            std::cout << "result is " << result << std::endl;
            return result;
        }
        // ��ѯ���ʧ��
        return -1;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}



int MysqlDao::ResetUser(const std::string& email, const std::string& passwd)
{
    auto conn = _pool->GetConnection();
    try {
        if (conn == nullptr) {
            _pool->ReturnConnection(std::move(conn));
            return -1;
        }

        const Defer defer([this, &conn]() {
            _pool->ReturnConnection(std::move(conn));
            });

        // ִ��MySQL����
        std::unique_ptr<sql::PreparedStatement> stmt(conn->_con->prepareStatement("CALL reset_user(?,?,@result)"));
        stmt->setString(1, email);
        stmt->setString(2, passwd);
        stmt->execute();

        // ����statement��ѯ���
        std::unique_ptr<sql::Statement> stmtResult(conn->_con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmtResult->executeQuery("select @result AS result"));
        // ��ѯ����ɹ�
        if (rs->next()) {
            int result = rs->getInt("result");
            std::cout << "result is " << result << std::endl;
            return result;
        }
        // ��ѯ���ʧ��
        return -1;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}


bool MysqlDao::checkPasswd(const std::string& name, const std::string& passwd, UserInfo& user_info) {
    auto connect = _pool->GetConnection();
    Defer defer([this, &connect] {
        _pool->ReturnConnection(std::move(connect));
        });

    try {
        if (connect == nullptr) {
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            connect->_con->prepareStatement("select * from user where name = ? and passwd = ?")
        );
        pstmt->setString(1, name);
        pstmt->setString(2, passwd);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        // if result set is empty, return false
        if (!res->next()) {
            std::cout << "password is not matched" << std::endl;
            return false;
        }

        user_info.name = res->getString("name");
        user_info.uid = res->getInt("uid");
        user_info.email = res->getString("email");
        user_info.passwd = passwd;
        return true;
    }

    catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}