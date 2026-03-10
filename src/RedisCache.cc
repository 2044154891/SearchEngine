#include "RedisCache.h"
#include "Logger.h"

RedisCache::~RedisCache() {
    _redis.reset();
}

RedisCache& RedisCache::getInstance() {
    static RedisCache instance;
    return instance;
}

bool RedisCache::init(const std::string& host, int port) {
    try {
        // 创建Redis连接选项
        sw::redis::ConnectionOptions connection_options;
        connection_options.host = host;
        connection_options.port = port;
        
        // 创建连接池选项
        sw::redis::ConnectionPoolOptions pool_options;
        pool_options.size = 3;  // 连接池大小
        
        // 创建Redis客户端（带连接池）
        _redis = std::make_unique<sw::redis::Redis>(
            sw::redis::Redis(connection_options, pool_options)
        );
        
        // 测试连接
        _redis->ping();
        
        _connected = true;
        INFO("Redis连接成功！");
        return true;
    } catch (const std::exception& e) {
        ERROR("Redis连接失败: %s", e.what());
        _connected = false;
        return false;
    }
}

void RedisCache::set(const std::string& key, const std::string& value, int ttl) {
    if (!_connected || !_redis) return;
    
    try {
        _redis->set(key, value, std::chrono::seconds(ttl));
    } catch (const std::exception& e) {
        ERROR("Redis set error: %s", e.what());
    }
}

std::string RedisCache::get(const std::string& key) {
    if (!_connected || !_redis) return "";
    
    try {
        auto value = _redis->get(key);
        if (value) {
            return *value;
        }
    } catch (const std::exception& e) {
        ERROR("Redis get error: %s", e.what());
    }
    
    return "";
}

void RedisCache::del(const std::string& key) {
    if (!_connected || !_redis) return;
    
    try {
        _redis->del(key);
    } catch (const std::exception& e) {
        ERROR("Redis del error: %s", e.what());
    }
}

bool RedisCache::exists(const std::string& key) {
    if (!_connected || !_redis) return false;
    
    try {
        return _redis->exists(key);
    } catch (const std::exception& e) {
        ERROR("Redis exists error: %s", e.what());
    }
    
    return false;
}

// 网页内容缓存相关方法
void RedisCache::setPageContent(int docId, const std::string& content, int ttl) {
    std::string key = "page:" + std::to_string(docId);
    set(key, content, ttl);
}

std::string RedisCache::getPageContent(int docId) {
    std::string key = "page:" + std::to_string(docId);
    return get(key);
}

// 搜索结果缓存相关方法
void RedisCache::setSearchResult(const std::string& query, const std::string& result, int ttl) {
    // 对查询进行简单的hash处理作为key
    std::string key = "search:" + query;
    set(key, result, ttl);
}

std::string RedisCache::getSearchResult(const std::string& query) {
    std::string key = "search:" + query;
    return get(key);
}
