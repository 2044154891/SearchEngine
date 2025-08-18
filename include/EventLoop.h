#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类  主要包含两大模块 Channel Poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    //在当前loop中执行
    void runInLoop(Functor cb);
    //把上层的注册的回调函数cb放入队列中，唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    //通过eventfd唤醒loop所在的线程
    void wakeup();

    //EventLoop的方法 =》 poller的方法
    void updateChannel(Channel * cahnnel);
    void removeChannel(Channel * channel);
    bool hasChannel(Channel * channel);

    //判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const
    {
        return threadID_ == CurrentThread::tid();//threadID_为创建时tid,tid()是返回当前线程tid
    }

private:
    void handleRead();
    // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调
    // 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait

    void doPendingFunctors(); // 执行上层回调

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; // 原子操作， 底层通过CAS实现
    std::atomic_bool quit_;    // 标志退出loop循环

    const pid_t threadID_; // 记录当前EventLoop是被na个线程创建的 即标识了当前EventLoop的所属线程

    Timestamp pollReturnTime_; // Poller返回发生事件的channels 的时间点

    std::unique_ptr<Poller> poller_; //管理的Poller

    int wakeupFd_; // 作用：当mainLoop获取一个新用户的Channel 需要通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel
    std::unique_ptr<Channel> wakeupChannel_; //封装wakeupFd_

    ChannelList activeChannels_; // 返回poller检测到当前所有有事件发生的Channel列表

    std::atomic_bool callingPendingFunctors_; // 表示当前的loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有回调操作
    std::mutex mutex_;                        // 互斥锁 ，用来保护上面的pendingFunctors_的线程安全
};