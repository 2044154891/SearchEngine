#include "ProtocolParser.h"
#include "Logger.h"
#include "Buffer.h"
#include "Logger.h"
#include "KeyRecommander.h"
#include "WebPageSearcher.h"
#include "RedisCache.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

ProtocolParser::ProtocolParser(EventLoop* ioLoop)
    : _ioLoop(ioLoop)
    , _webPageSearcher(new WebPageSearcher())
    , _threadPool(new SimpleThreadPool(4, []() {
        // 每个计算线程初始化时创建独立的Redis连接
        auto& redis = RedisCache::getInstance();
        redis.init("127.0.0.1", 6379);
        INFO("计算线程: Redis连接已创建");
    }))
{
    // 加载索引（只在服务器启动时加载一次）
    _webPageSearcher->loadIndex();
    
    // 注册默认的任务处理器（使用异步处理）
    registerTaskHandler(TASK_RECOMMEND_KEYWORDS, 
        [this](const TcpConnectionPtr& conn, const std::string& content) {
            handleRecommendKeywordsAsync(conn, content);
        });
    
    registerTaskHandler(TASK_SEARCH_WEBPAGES, 
        [this](const TcpConnectionPtr& conn, const std::string& content) {
            handleSearchWebpagesAsync(conn, content);
        });
}

ProtocolParser::~ProtocolParser() {
}

bool ProtocolParser::tryParse(Buffer* buf, uint32_t& msgId, std::string& content) {
    if (buf->readableBytes() < kMessageLen + kMessageId) {
        return false;
    }

    uint32_t messageLen = 0;
    uint32_t messageId = 0;
    const char* p = buf->peek();
    std::memcpy(&messageLen, p, 4);
    std::memcpy(&messageId, p + 4, 4);
    uint32_t bodyLen = ntohl(messageLen);
    msgId = ntohl(messageId);

    //防御阈值
    if (bodyLen > 100 * 1024 * 1024) {
        FATAL("message overflow the maxsize\n");
        return false;
    }

    if (buf->readableBytes() < kMessageLen + kMessageId + bodyLen) return false;

    buf->retrieve(kMessageLen + kMessageId);
    content = buf->retrieveAsString(bodyLen);
    return true;
    
}

void ProtocolParser::parseAndDispatch(const TcpConnectionPtr& conn, Buffer* buf) {
    std::cout << "parseAndDispatch called, buffer readable bytes: " << buf->readableBytes() << std::endl;
    
    uint32_t msgId = 0;
    std::string content;
    if (!tryParse(buf, msgId, content)) {
        std::cout << "tryParse failed, not enough data" << std::endl;
        return;
    }
    
    std::cout << "Parsed message: ID=" << msgId << ", content='" << content << "'" << std::endl;
    
    int taskId = static_cast<int>(msgId);
    auto it = _taskHandlers.find(taskId);
    if (it != _taskHandlers.end()) {
        std::cout << "Found handler for task ID: " << taskId << std::endl;
        it->second(conn, content);
    } else {
        std::cout << "No handler found for task ID: " << taskId << std::endl;
        if (_defaultHandler) {
            _defaultHandler(conn, content);
        } else {
            sendFrame(conn, 0, "ERROR : Unknown task ID" + std::to_string(taskId));
        }
    }
}

void ProtocolParser::sendFrame(const TcpConnectionPtr& conn, uint32_t msgId, const std::string& content) {
    std::cout << "sendFrame: sending ID=" << msgId << ", content='" << content << "'" << std::endl;
    
    uint32_t lenNet = htonl(static_cast<uint32_t>(content.size()));
    uint32_t idNet  = htonl(msgId);

    std::string out;
    out.resize(8 + content.size());
    std::memcpy(&out[0], &lenNet, 4);
    std::memcpy(&out[4], &idNet, 4);
    if (!content.empty()) {
        std::memcpy(&out[8], content.data(), content.size());
    }
    
    std::cout << "sendFrame: sending " << out.size() << " bytes" << std::endl;
    conn->send(out);
    std::cout << "sendFrame: sent successfully" << std::endl;
}

void ProtocolParser::registerTaskHandler(int taskId, TaskHandler handler) {
    _taskHandlers[taskId] = handler;
}

void ProtocolParser::setDefaultHandler(TaskHandler handler) {
    _defaultHandler = handler;
}


// 任务1：处理推荐关键词请求
void ProtocolParser::handleRecommendKeywords(const TcpConnectionPtr& conn, const std::string& content) {
    KeyRecommander recommander(content);
    recommander.execute();
    const auto &full = recommander.suggestions();

    nlohmann::json j;
    j["id"] = RESPONSE_RECOMMEND_KEYWORDS; // 100
    j["query"] = content;
    j["suggestions"] = full;

    sendFrame(conn, RESPONSE_RECOMMEND_KEYWORDS, j.dump());
}

// 任务2：处理网页搜索请求
void ProtocolParser::handleSearchWebpages(const TcpConnectionPtr& conn, const std::string& content) {
    std::cout << "Handling search webpages request: " << content << std::endl;
    
    // 使用 WebPageSearcher 进行真正的搜索
    auto results = _webPageSearcher->search(content, 10);
    
    // 构造JSON响应
    nlohmann::json j;
    j["id"] = RESPONSE_SEARCH_WEBPAGES;
    j["query"] = content;
    j["total"] = results.size();
    j["results"] = nlohmann::json::array();
    
    for (const auto& result : results) {
        nlohmann::json item;
        item["docId"] = result.docId;
        item["score"] = result.score;
        item["title"] = result.title;
        item["url"] = result.url;
        item["summary"] = result.summary;
        j["results"].push_back(item);
    }
    
    sendFrame(conn, RESPONSE_SEARCH_WEBPAGES, j.dump());
}

// 异步任务1：处理推荐关键词请求（在新线程中执行计算）
void ProtocolParser::handleRecommendKeywordsAsync(const TcpConnectionPtr& conn, const std::string& content) {
    // 捕获需要的变量副本，避免跨线程引用问题
    // 使用weak_ptr来检测连接是否仍然有效
    std::weak_ptr<TcpConnection> weakConn = conn;
    auto ioLoop = _ioLoop;
    
    // 将计算任务提交到线程池
    _threadPool->enqueue([weakConn, ioLoop, content]() {
        std::cout << "[ThreadPool] handleRecommendKeywordsAsync executing for: " << content << std::endl;
        
        // 在计算线程中执行推荐算法
        KeyRecommander recommander(content);
        recommander.execute();
        const auto& full = recommander.suggestions();

        // 构造JSON响应
        nlohmann::json j;
        j["id"] = RESPONSE_RECOMMEND_KEYWORDS;
        j["query"] = content;
        j["suggestions"] = full;
        std::string responseStr = j.dump();

        // 通过IO线程回调发送响应（线程安全）
        auto connPtr = weakConn.lock();  // 尝试获取shared_ptr
        if (connPtr) {
            ioLoop->runInLoop([connPtr, responseStr]() {
                // 在IO线程中发送响应
                uint32_t lenNet = htonl(static_cast<uint32_t>(responseStr.size()));
                uint32_t idNet  = htonl(RESPONSE_RECOMMEND_KEYWORDS);

                std::string out;
                out.resize(8 + responseStr.size());
                std::memcpy(&out[0], &lenNet, 4);
                std::memcpy(&out[4], &idNet, 4);
                if (!responseStr.empty()) {
                    std::memcpy(&out[8], responseStr.data(), responseStr.size());
                }
                
                connPtr->send(out);
                std::cout << "[ThreadPool] Response sent for recommend keywords" << std::endl;
            });
        } else {
            std::cout << "[ThreadPool] Connection already closed, skipping response" << std::endl;
        }
    });
}

// 异步任务2：处理网页搜索请求（在新线程中执行计算）
void ProtocolParser::handleSearchWebpagesAsync(const TcpConnectionPtr& conn, const std::string& content) {
    // 捕获需要的变量副本
    std::weak_ptr<TcpConnection> weakConn = conn;
    auto ioLoop = _ioLoop;
    auto webPageSearcher = _webPageSearcher;
    
    // 将计算任务提交到线程池
    _threadPool->enqueue([weakConn, ioLoop, content, webPageSearcher]() {
        std::cout << "[ThreadPool] handleSearchWebpagesAsync executing for: " << content << std::endl;
        
        // 在计算线程中执行搜索
        auto results = webPageSearcher->search(content, 10);
        
        // 构造JSON响应
        nlohmann::json j;
        j["id"] = RESPONSE_SEARCH_WEBPAGES;
        j["query"] = content;
        j["total"] = results.size();
        j["results"] = nlohmann::json::array();
        
        for (const auto& result : results) {
            nlohmann::json item;
            item["docId"] = result.docId;
            item["score"] = result.score;
            item["title"] = result.title;
            item["url"] = result.url;
            item["summary"] = result.summary;
            j["results"].push_back(item);
        }
        
        std::string responseStr = j.dump();
        
        // 写入Redis缓存（每个计算线程有独立Redis连接）
        auto& redis = RedisCache::getInstance();
        if (redis.isConnected()) {
            redis.setSearchResult(content, responseStr, 1800);
        }

        // 通过IO线程回调发送响应（线程安全）
        auto connPtr = weakConn.lock();
        if (connPtr) {
            ioLoop->runInLoop([connPtr, responseStr]() {
                uint32_t lenNet = htonl(static_cast<uint32_t>(responseStr.size()));
                uint32_t idNet  = htonl(RESPONSE_SEARCH_WEBPAGES);

                std::string out;
                out.resize(8 + responseStr.size());
                std::memcpy(&out[0], &lenNet, 4);
                std::memcpy(&out[4], &idNet, 4);
                if (!responseStr.empty()) {
                    std::memcpy(&out[8], responseStr.data(), responseStr.size());
                }
                
                connPtr->send(out);
                std::cout << "[ThreadPool] Response sent for search webpages, size: " << responseStr.size() << std::endl;
            });
        } else {
            std::cout << "[ThreadPool] Connection already closed, skipping response" << std::endl;
        }
    });
}
