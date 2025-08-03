const config_module = require('./config.js');
const redis = require('ioredis')
// 创建Redis客户端
const redisClient = new redis(
    {
        host : config_module.redis_host,
        port : config_module.redis_port,
        password : config_module.redis_passwd,
    }
);

// 监听错误信息
redisClient.on("error", function(error){
    console.log("Redis connection error: ", error);
    redisClient.quit();
});

// 获取Redis中的值
async function getRedis(key){
    try{
        const result = await redisClient.get(key);
        if(result == null){
            console.log("Key not found in Redis:", key);
            return null; // 如果键不存在，返回null
        }
        console.log("Redis key:", key, "Value:", result);
        return result; // 返回键对应的值
    }
    catch(error){
        console.log("Error getting Redis key:", error);
        return null; // 如果发生错误，返回null
    }
}

async function queryRedis(key){
    try{
        const result = await redisClient.get(key);
        if(result === 0){
            console.log("Key not found in Redis:", key);
            return null; // 如果键不存在，返回null
        }
        console.log("Redis key:", key, "Value:", result);
        return result; // 返回键对应的值
    }
    catch(error){
        console.log("Error getting Redis key:", error);
        return null; // 如果发生错误，返回null
    }
}

async function setRedisExpire(key, value, expireTime){
    try{
        if(expireTime <= 0){
            console.log("Expire time must be greater than 0");
            return;
        }
        // 设置键值对并设置过期时间
        await redisClient.set(key, value);
        await redisClient.expire(key, expireTime);
        console.log("Set Redis key:", key, "Value:", value, "Expire time:", expireTime);
        return true;
    }
    catch(error){
        console.log("Error setting Redis key:", error);
    }
}

function closeRedis(){
    redisClient.quit();
    console.log("Redis connection closed");
}

module.exports = {getRedis, queryRedis, setRedisExpire, closeRedis};