#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include "Buffer.h"
#include "TcpConnection.h"
#include "KeyRecommander.h"
#include <functional>
#include <string>
#include <map>

// 前向声明
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

// 任务处理函数类型定义
using TaskHandler = std::function<void(const TcpConnectionPtr&, const std::string&)>; //在onmessage中设置回调函数

class ProtocolParser
{
public:
    ProtocolParser();
    ~ProtocolParser();
    
    // 解析消息并分发任务
    void parseAndDispatch(const TcpConnectionPtr& conn, Buffer* buf);
    
    // 注册任务处理器
    void registerTaskHandler(int taskId, TaskHandler handler);
    
    // 设置默认任务处理器
    void setDefaultHandler(TaskHandler handler);

private:
    // 解析消息格式：提取任务ID和消息内容
    bool parseMessage(const std::string& rawMsg, int& taskId, std::string& content); //解析字段

    //按帧协议尝试解析一条消息，并在成功时从buf中retrive出消息内容
    bool tryParse(Buffer* buf, uint32_t& msgId, std::string& content);

    //按帧协议发送消息
    void sendFrame(const TcpConnectionPtr& conn, uint32_t msgId, const std::string& content);
    
    // 任务处理器映射表
    std::map<int, TaskHandler> _taskHandlers;
    
    // 默认任务处理器
    TaskHandler _defaultHandler;
    
    // 任务/响应ID常量定义（constexpr避免ODR定义问题）
    static constexpr int TASK_RECOMMEND_KEYWORDS = 1;    // 推荐关键词任务
    static constexpr int TASK_SEARCH_WEBPAGES   = 2;     // 搜索网页任务
    static constexpr int RESPONSE_RECOMMEND_KEYWORDS = 100;  // 推荐关键词响应
    static constexpr int RESPONSE_SEARCH_WEBPAGES     = 200; // 搜索网页响应

    static constexpr size_t kMessageLen = 4;
    static constexpr size_t kMessageId  = 4;
    
    // 具体的任务处理方法
    void handleRecommendKeywords(const TcpConnectionPtr& conn, const std::string& content);
    void handleSearchWebpages(const TcpConnectionPtr& conn, const std::string& content);
};

#endif