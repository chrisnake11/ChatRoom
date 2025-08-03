#pragma once
#include <hiredis.h>
#include "Singleton.h"
#include <queue>
#include <functional>
#include "Const.h"
#include "ConfigManager.h"

class RedisPool {
public:
	RedisPool(std::size_t pool_size, const char* host, int port, const char* pwd);
	redisContext* GetConnection();
	void ReturnConnection(redisContext* conn);
	void Close();
	~RedisPool();
	//void CheckRedisPool();

private:
	std::queue<redisContext*> _redis_pool;
	std::size_t _pool_size;
    std::mutex _mutex;
    std::condition_variable _cond;
	std::atomic<bool> _b_stop;
	const char* _host;
	int _port;

	/*const char* _pwd;
	std::atomic<int> _fail_count;
	int _counter;
	std::thread  check_thread_;*/
};

class RedisManager : public Singleton<RedisManager> {
	friend class Singleton<RedisManager>;
public:
	~RedisManager();
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool Del(const std::string& key);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& field, const std::string& value);
	bool HSet(const char* key, const char* field, const char* value, size_t value_len);
	std::string HGet(const std::string& key, const std::string& field);
	std::string HGet(const char* key, const char* field);
	bool HDel(const std::string& key, const std::string& field);
	bool ExistKey(const std::string& key);

	void Close();

	/*std::string AcquireLock(const std::string& lock_name, int lock_timeout, int acquire_timeout);

	bool ReleaseLock(const std::string& lock_name, const std::string& lock_token);

	void IncreaseCounter(std::string server_name);
	void DecreaseCounter(std::string server_name);
	void InitCounter(std::string server_name);
	void DelCounter(std::string server_name);*/

private:
	RedisManager();
	std::unique_ptr<RedisPool>  _redis_pool;
};