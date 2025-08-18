#pragma once

#include <functional>
#include <string>
#include <vector>

#include "noncopyable.h"
#include "EventLoopThread.h"
#include "EventLoop.h"

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop * baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    //如果工作再多线程中，baseLoop_(mainLoop) 会默认以轮询的方式分配Channel给subLoop
    EventLoop * getNextLoop();

    std::vector<EventLoop *> getALLloops(); //获取所有的EventLoop

    bool strated() const { return started_;} //是否已经启动
    const std::string name() const { return name_; }
private:
    EventLoop * baseLoop_; //用户使用muduo创建的Loop,if 线程数为1，就直接使用用户创建的loop,否则创建多EventLoop
    std::string name_; //线程池名称，通常由用户指定，线程中EventLoopThread名称依赖于线程池名称
    bool started_; //是否启动标志
    int numThreads_; //线程池中线程的数量
    int next_ ; //新连接的到来，所以选择EventLoop索引
    std::vector<std::unique_ptr<EventLoopThread> > threads_; //IO线程的列表
    std::vector<EventLoop *> loops_; // 线程池中EventLoop的列表， 指向的是EventLoopThread线程函数创建的EventLoop对象
};