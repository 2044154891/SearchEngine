#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include "Buffer.h"
#include "TcpConnection.h"
#include "KeyRecommander.h"
#include "WebPageSearcher.h"
#include "EventLoopThreadPool.h"
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

// 前向声明
class TcpConnection;
class EventLoop;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

// 任务处理函数类型定义
using TaskHandler = std::function<void(const TcpConnectionPtr&, const std::string&)>; //在onmessage中设置回调函数

// 简单的线程池实现（支持线程初始化回调）
class SimpleThreadPool {
public:
    using ThreadInitCallback = std::function<void()>;
    
    SimpleThreadPool(size_t numThreads, ThreadInitCallback initCallback = nullptr) 
        : _stop(false), _initCallback(initCallback) {
        for (size_t i = 0; i < numThreads; ++i) {
            _workers.emplace_back([this] {
                // 每个线程初始化时执行回调（如创建Redis连接）
                if (_initCallback) {
                    _initCallback();
                }
                
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(_queueMutex);
                        _condition.wait(lock, [this] { return _stop.load() || !_tasks.empty(); });
                        if (_stop.load() && _tasks.empty()) return;
                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~SimpleThreadPool() {
        _stop.store(true);
        _condition.notify_all();
        for (auto& worker : _workers) {
            if (worker.joinable()) worker.join();
        }
    }

    template<typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _tasks.emplace(std::forward<F>(f));
        }
        _condition.notify_one();
    }

private:
    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;
    std::mutex _queueMutex;
    std::condition_variable _condition;
    std::atomic<bool> _stop;
    ThreadInitCallback _initCallback;  // 线程初始化回调
};

class ProtocolParser
{
public:
    explicit ProtocolParser(EventLoop* ioLoop);
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

    //按帧协议发送消息（线程安全，在IO线程中执行）
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

    // 网页搜索引擎（使用指针，在IO线程中初始化）
    std::shared_ptr<WebPageSearcher> _webPageSearcher;

    static constexpr size_t kMessageLen = 4;
    static constexpr size_t kMessageId  = 4;
    
    // IO线程EventLoop（用于跨线程回调）
    EventLoop* _ioLoop;
    
    // 计算线程池
    std::unique_ptr<SimpleThreadPool> _threadPool;
    
    // 具体的任务处理方法（同步版本，用于注册）
    void handleRecommendKeywords(const TcpConnectionPtr& conn, const std::string& content);
    void handleSearchWebpages(const TcpConnectionPtr& conn, const std::string& content);
    
    // 异步任务处理方法（在新线程中执行计算）
    void handleRecommendKeywordsAsync(const TcpConnectionPtr& conn, const std::string& content);
    void handleSearchWebpagesAsync(const TcpConnectionPtr& conn, const std::string& content);
};

#endif
