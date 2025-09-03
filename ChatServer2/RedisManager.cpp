#include "RedisManager.h"

RedisManager::RedisManager()
{
    std::cout << "RedisManager init..." << std::endl;
    auto& config = ConfigManager::GetInstance();
    auto host = config["Redis"]["Host"];
    auto port = config["Redis"]["Port"];
    auto pwd = config["Redis"]["Password"];
    _redis_pool.reset(new RedisPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisManager::~RedisManager()
{
    Close();
}

void RedisManager::Close() {
    _redis_pool->Close();
}

bool RedisManager::Get(const std::string& key, std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        std::cout << "Get Redis connection failed." << std::endl;
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    // 返回类型为void*，需要强转成redisReply*
    reply = (redisReply*)redisCommand(connection, "GET %s", key.c_str());

    // 检查执行结果
    if (reply == nullptr) {
        std::cout << "[ GET " << key << "] failed." << std::endl;
        return false;
    }

    // 检查返回类型
    if (reply->type != REDIS_REPLY_STRING){
        std::cout << "[ GET " << key << "] is not string type." << std::endl;
        return false;
    }

    value = reply->str;
    std::cout << "Success to execute command [ GET " << key << "]." << std::endl;
    return true;
}

bool RedisManager::Set(const std::string& key, const std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    // 返回类型为void*，需要强转成redisReply*
    reply = (redisReply*)redisCommand(connection, "SET %s %s", key.c_str(), value.c_str());

    // 检查执行结果
    if (reply == nullptr) {
        std::cout << "[ SET " << key << " " << value << " ] failed." << std::endl;
        return false;
    }

    // 只有返回成功且是OK才成功
    if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") || strcmp(reply->str, "ok")))) {
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
        return false;
    }

    std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
    return true;
}

bool RedisManager::LPush(const std::string& key, const std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    reply = (redisReply*)redisCommand(connection, "LPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failed ! " << std::endl;
        return false;
    }

    // 只有返回值是整数且大于0才算成功
    if (!(reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) ) {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failed ! " << std::endl;
        return false;
    }

    std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
    return true;
}

bool RedisManager::RPush(const std::string& key, const std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    reply = (redisReply*)redisCommand(connection, "RPush %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        std::cout << "Execut command [ RPush " << key << "  " << value << " ] failed ! " << std::endl;
        return false;
    }

    // 只有返回值是整数且大于0才算成功
    if (!(reply->type == REDIS_REPLY_INTEGER && reply->integer > 0)) {
        std::cout << "Execut command [ RPush " << key << "  " << value << " ] failed ! " << std::endl;
        return false;
    }

    std::cout << "Execut command [ RPush " << key << "  " << value << " ] success ! " << std::endl;
    return true;
}

bool RedisManager::LPop(const std::string& key, std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    reply = (redisReply*)redisCommand(connection, "LPOP %s", key.c_str());
    if (reply == nullptr) {
        return false;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        std::cout << "LPOP " << key << " failed! " << std::endl;
        return false;
    }

    value = reply->str;
    return true;
}

bool RedisManager::RPop(const std::string& key, std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    reply = (redisReply*)redisCommand(connection, "RPop %s", key.c_str());
    if (reply == nullptr) {
        return false;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        std::cout << "RPop " << key << " failed! " << std::endl;
        return false;
    }

    value = reply->str;
    return true;
}

bool RedisManager::HSet(const std::string& key, const std::string& field, const std::string& value)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        }); 
    
    reply = (redisReply*)redisCommand(connection, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    if (reply == nullptr) {
        std::cout << "Execut command [ HSET " << key << "  " << field << " " << value << " ] failed ! " << std::endl;
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ HSET " << key << "  " << field << " " << value << " ] failed ! " << std::endl;
        return false;
    }

    std::cout << "Execut command [ HSET " << key << "  " << field << " " << value << " ] success ! " << std::endl;
    return true;
}

bool RedisManager::HSet(const char* key, const char* field, const char* value, size_t value_len) {
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = field;
    argvlen[2] = strlen(field);
    argv[3] = value;
    argvlen[3] = value_len;

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    // 允许将命令参数，通过字符数组的形式传递。
    reply = (redisReply*)redisCommandArgv(connection, 4, argv, argvlen);
    if (reply == nullptr) {
        std::cout << "Execut command [ HSET " << key << "  " << field << " " << value << " ] failed ! " << std::endl;
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER){
        std::cout << "Execut command [ HSET " << key << "  " << field << " " << value << " ] failed ! " << std::endl;
        return false;
    }

    std::cout << "Execut command [ HSET " << key << "  " << field << " " << value << " ] success ! " << std::endl;
    return true;
}

std::string RedisManager::HGet(const std::string& key, const std::string& field)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return "";
    }

    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = field.c_str();
    argvlen[2] = field.length();


    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommandArgv(connection, 3, argv, argvlen);
    if (reply == nullptr) {
        std::cout << "Execut command [ HGET " << key << "  " << field << " ] failed ! " << std::endl;
        return "";
    }

    if (reply->type == REDIS_REPLY_NIL) {
        std::cout << "HGET " << key << " " << field << " failed! " << std::endl;
        return "";
    }

    std::string value = reply->str;
    std::cout << "Execut command [ HGET " << key << "  " << field << " ] success ! " << std::endl;
    return value;
}

std::string RedisManager::HGet(const char* key, const char* field) {
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return "";
    }

    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = field;
    argvlen[2] = strlen(field);


    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommandArgv(connection, 3, argv, argvlen);
    if (reply == nullptr) {
        std::cout << "Execut command [ HGET " << key << "  " << field << " ] failed ! " << std::endl;
        return "";
    }

    if (reply->type == REDIS_REPLY_NIL) {
        std::cout << "HGET " << key << " " << field << " failed! " << std::endl;
        return "";
    }

    std::string value = reply->str;
    std::cout << "Execut command [ HGET " << key << "  " << field << " ] success ! " << std::endl;
    return value;
}

bool RedisManager::HDel(const std::string& key, const std::string& field)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }


    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommand(connection, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) {
        std::cout << "Execut command [ HDEL " << key << "  " << field << " ] failed ! " << std::endl;
        return false;
    }

    bool success = false;
    // integer > 0 表示删除成功
    if (reply->type == REDIS_REPLY_INTEGER) {
        success = reply->integer > 0;
    }

    return success;
}

bool RedisManager::Del(const std::string& key) {
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommand(connection, "DEL %s", key.c_str());
    if (reply == nullptr) {
        std::cout << "Execut command [ DEL " << key << " ] failed ! " << std::endl;
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ DEL " << key << " ] failed ! " << std::endl;
        return false;
    }

    std::cout << "Execut command [ DEL " << key << " ] success ! " << std::endl;
    return true;
}

bool RedisManager::ExistKey(const std::string& key) {
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommand(connection, "EXISTS %s", key.c_str());

    if (reply == nullptr) {
        std::cout << "Execut command [ EXISTS " << key << " ] failed ! " << std::endl;
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        std::cout << "Not Found [ " << key << " ] ! " << std::endl;
        return false;
    }

    std::cout << "Found [ key " << key << " ] ! " << std::endl;
    return true;
}

bool RedisManager::LRange(const std::string& key, int start, int end, std::vector<std::string>& values) {
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommand(connection, "LRANGE %s %d %d", key.c_str(), start, end);

    if (reply == nullptr) {
        std::cout << "LRange " << key << " [ " << start << ", " << end << "] failed !" << std::endl;
        return false;
    }

    // 返回类型不是数组
    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cout << "Not Found [ " << key << " [ " << start << ", " << end << "] " << std::endl;
        return false;
    }

    values.clear();

    for (size_t i = 0; i < reply->elements; ++i) {
        values.push_back(reply->element[i]->str);
    }

    std::cout << "Found [ " << key << " [ " << start << ", " << end << "] " << std::endl;
    return true;
}

bool RedisManager::LTrim(const std::string& key, int start, int end)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommand(connection, "LTRIM %s %d %d", key.c_str(), start, end);

    if (reply == nullptr) {
        std::cout << "LTrim " << key << " [ " << start << ", " << end << "] failed !" << std::endl;
        return false;
    }

    // 返回类型不是数组
    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cout << "Not Found Trim [ " << key << " [ " << start << ", " << end << "] " << std::endl;
        return false;
    }

    std::cout << "Found Trim [ " << key << " [ " << start << ", " << end << "] " << std::endl;
    return true;
}

bool RedisManager::LRem(const std::string& key, int index, const std::string& str_item)
{
    auto connection = _redis_pool->GetConnection();
    if (connection == nullptr) {
        return false;
    }

    redisReply* reply = nullptr;
    Defer defer([&reply, &connection, this]() {
        if (reply) freeReplyObject(reply);
        _redis_pool->ReturnConnection(connection);
        });

    reply = (redisReply*)redisCommand(connection, "LREM %s %d %s", key.c_str(), index, str_item.c_str());

    if (reply == nullptr) {
        std::cout << "LTrim " << key << " [ " << index << ", " << str_item << "] failed !" << std::endl;
        return false;
    }

    // 返回类型不是数组
    if (reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Not Found Trim [ " << key << " [ " << index << ", " << str_item << "] " << std::endl;
        return false;
    }

    std::cout << "LREM " << key << " count=" << index << " value=" << str_item
        << " deleted=" << reply->integer << std::endl;
    return true;
}


RedisPool::RedisPool(std::size_t pool_size, const char* host, int port, const char* pwd)
    : _pool_size(pool_size), _host(host), _port(port), _b_stop(false)
{
    std::cout << "RedisPool init..." << std::endl;
    // 初始化redis连接，认证并添加到连接池
    for (std::size_t i = 0; i < pool_size; ++i) {
        auto* context = redisConnect(_host, _port);
        if (context == nullptr || context->err != 0) {
            if (context != nullptr) {
                redisFree(context);
            }
            continue;
        }

        auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
        if (reply == nullptr) {
            std::cout << "Redis Connect Failed ! " << std::endl;
            freeReplyObject(reply);
            continue;
        }

        if (reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Redis Autorization Failed!" << std::endl;
            freeReplyObject(reply);
            continue;
        }

        freeReplyObject(reply);
        std::cout << "Redis Authorize Success" << std::endl;
        _redis_pool.push(context);
    }
}



// 析构时，释放连接池中的所有连接
RedisPool::~RedisPool()
{
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_redis_pool.empty()) {
        _redis_pool.pop();
    }
}

redisContext* RedisPool::GetConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);
    
    _cond.wait(lock, [this] {
        if (_b_stop) {
            return true;
        }
        return !_redis_pool.empty();
        });

    if (_b_stop) {
        return nullptr;
    }

    auto * context = _redis_pool.front();
    _redis_pool.pop();
    return context;
}

void RedisPool::ReturnConnection(redisContext* conn)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_b_stop) {
        return;
    }
    _redis_pool.push(conn);
    _cond.notify_one();
}

void RedisPool::Close() {
    _b_stop = true;
    _cond.notify_all();
}


