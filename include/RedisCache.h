#pragma once

#include <string>
#include <memory>
#include <hiredis/hiredis.h>
#include <sw/redis++/redis++.h>

class RedisCache {
public:
    // 获取单例实例
    static RedisCache& getInstance();

    // 初始化Redis连接
    bool init(const std::string& host = "127.0.0.1", int port = 6379);

    // 检查是否已连接
    bool isConnected() const { return _connected; }

    // 设置缓存（带TTL）
    void set(const std::string& key, const std::string& value, int ttl = 3600);

    // 获取缓存
    std::string get(const std::string& key);

    // 删除缓存
    void del(const std::string& key);

    // 检查key是否存在
    bool exists(const std::string& key);

    // 网页内容缓存相关方法
    void setPageContent(int docId, const std::string& content, int ttl = 3600);
    std::string getPageContent(int docId);

    // 搜索结果缓存相关方法
    void setSearchResult(const std::string& query, const std::string& result, int ttl = 1800);
    std::string getSearchResult(const std::string& query);

private:
    RedisCache() : _connected(false) {}
    ~RedisCache();

    // 禁止拷贝
    RedisCache(const RedisCache&) = delete;
    RedisCache& operator=(const RedisCache&) = delete;

    std::unique_ptr<sw::redis::Redis> _redis;
    bool _connected;
};
