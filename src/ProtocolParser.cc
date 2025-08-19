#include "ProtocolParser.h"
#include "Buffer.h"
#include "Logger.h"
#include "KeyRecommander.h"
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
    uint32_t msgId = 0;
    std::string content;
    if (!tryParse(buf, msgId, content)) {
        //数据不够
        return;
    }
    int taskId = static_cast<int>(msgId);
    auto it = _taskHandlers.find(taskId);
    if (it != _taskHandlers.end()) {
        it->second(conn, content);
    } else {
        if (_defaultHandler) {
            _defaultHandler(conn, content);
        } else {
            sendFrame(conn, 0, "ERROR : Unknown task ID" + std::to_string(taskId));
        }
    }
}

void ProtocolParser::sendFrame(const TcpConnectionPtr& conn, uint32_t msgId, const std::string& content) {
    uint32_t lenNet = htonl(static_cast<uint32_t>(content.size()));
    uint32_t idNet  = htonl(msgId);

    std::string out;
    out.resize(8 + content.size());
    std::memcpy(&out[0], &lenNet, 4);
    std::memcpy(&out[4], &idNet, 4);
    if (!content.empty()) {
        std::memcpy(&out[8], content.data(), content.size());
    }
    conn->send(out);
}

void ProtocolParser::registerTaskHandler(int taskId, TaskHandler handler) {
    _taskHandlers[taskId] = handler;
}

void ProtocolParser::setDefaultHandler(TaskHandler handler) {
    _defaultHandler = handler;
}


// 任务1：处理推荐关键词请求
void ProtocolParser::handleRecommendKeywords(const TcpConnectionPtr& conn, const std::string& content) {
    // 使用关键词推荐模块，内部已构造 JSON 并发送
    KeyRecommander recommander(content, conn);
    recommander.execute();
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
