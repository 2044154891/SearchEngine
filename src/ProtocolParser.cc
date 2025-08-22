#include "ProtocolParser.h"
#include "Buffer.h"
#include "Logger.h"
#include "KeyRecommander.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

ProtocolParser::ProtocolParser() {
    // 注册默认的任务处理器
    registerTaskHandler(TASK_RECOMMEND_KEYWORDS, 
        [this](const TcpConnectionPtr& conn, const std::string& content) {
            handleRecommendKeywords(conn, content);
        });
    
    registerTaskHandler(TASK_SEARCH_WEBPAGES, 
        [this](const TcpConnectionPtr& conn, const std::string& content) {
            handleSearchWebpages(conn, content);
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
        LOG_FATAL("message overflow the maxsize\n");
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
    
    // 这里实现网页搜索的逻辑
    // 例如：搜索包含关键词的网页信息
    
    // 构造响应消息：200:搜索到的网页信息
    std::string response = std::to_string(RESPONSE_SEARCH_WEBPAGES) + ":" + 
                          "搜索结果: " + content + " - 网页1,网页2,网页3";
    
    sendFrame(conn, RESPONSE_SEARCH_WEBPAGES, response);
}
