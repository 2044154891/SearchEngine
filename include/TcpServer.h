#pragma once

#include <functional>
#include <atomic>
#include <unordered_map>

// 一站式包含muduo所有常用头文件，用户只需包含TcpServer.h即可
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Logger.h"
#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "Channel.h"
#include "Poller.h"
#include "Socket.h"
#include "Thread.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "EPollPoller.h"

//对外的服务器编程使用的类
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort, //不允许重用本地端口 0
        kReusePort, //允  1
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    //设置底层subloop的个数
    void setThreadNum(int numThreads);
    
    /*  if 没有监听，就启动服务器(监听)
    多次调用没有副作用，
    线程safe
    */
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop * loop_; //baseloop用户定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; //运行在mainloop ,就是监听连接新事件

    std::unique_ptr<EventLoopThreadPool> threadPool_; //one loop per thread

    ConnectionCallback connectionCallback_;  //有新连接时的回调函数
    MessageCallback messageCallback_; //有读写事件发生的回调
    WriteCompleteCallback writeCompleteCallback_; //消息发送完成后的回调

    ThreadInitCallback threadInitCallback_; //loop 线程初始化的回调
    int numThreads_; //线程池中线程的数量
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_; //保存所有的连接
};