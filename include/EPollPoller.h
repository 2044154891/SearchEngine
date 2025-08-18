#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

/*
epoll 的使用
1.epoll_create()
2.epoll_ctl(add,mod,del)
3.epoll_wait
*/

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop * loop);
    ~EPollPoller() override;

    //重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList *activeChannels) override; //epoll_wait
    void updateChannel(Channel * channel) override; //epoll_ctl
    void removeChannel(Channel * channel) override; //epoll_ctl
private:
    static const int kInitEventListSize = 16;

    //填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    //更新channel通道，其实就是调用epoll_ctl
    void update(int operation, Channel * channel);

    using EventList = std::vector<epoll_event>; // cpp可省略struct

    int epollfd_; //epoll_create创建返回的fd保存在epollfd中
    EventList events_;//用于存放epoll_wait返回的所有发生的事件的文件描述符

};