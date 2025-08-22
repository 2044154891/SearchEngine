#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1; //某个channel还没有添加至Poller  //channel的成员index_初始化为-1
const int kAdded = 1; //某个channel已经添加至POller
const int kDeleted = 2; //某个channel已经从POller删除

EPollPoller::EPollPoller(EventLoop * loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) //EPOLL_CLOEXEC 标志用于确保创建的 epoll 文件描述符在 exec() 调用时会被自动关闭，防止描述符被新进程继承并引起潜在的资源管理问题。
    , events_(kInitEventListSize) // vector<epoll_event>(16)
{
    if(epollfd_ < 0){
         LOG_FATAL("epoll_create error: %d\n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    //由于频繁调用poll,实际上应该使用LOG_DEBUG输出log更为合理
    //当遇到并发场景， 关闭DEBUG log 提升效率
    //LOG_DEBUG("func=%s => fd total count:%lu\n",__FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_,&(*events_.begin()),static_cast<int>(events_.size()),timeoutMs);
    int saveError = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0){
        LOG_DEBUG("%d events happend\n",numEvents); //LOG_DEBUG最合理
        fillActiveChannels(numEvents,activeChannels);
        if(numEvents == events_.size()){
            //扩容操作
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents == 0){
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else{
        if(saveError != EINTR){
            errno = saveError;
            LOG_ERROR("EPollPoller::poll() error! :%s",strerror(saveError));
        }
    }
    return now;
}

//channel update/remove => Eventloop update/remove => Poller update/remove
void EPollPoller::updateChannel(Channel * channel)
{
    const int index = channel->index();  //获取channel在poller中的状态
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n",
    __FUNCTION__,channel->fd(),channel->events(),index);
    if(index == kNew || index == kDeleted){
        if(index == kNew){
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        else{//index == kDelete

        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else{//channel 已经在Poller中注册过了
        int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }else{
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

//从Poller中删除channel
void EPollPoller::removeChannel(Channel * channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n",__FUNCTION__,fd);

    int index = channel->index();
    if(index == kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

//填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,ChannelList * activeChannels) const
{
    for(int i = 0; i < numEvents; ++i){
        Channel * channel = static_cast<Channel * >(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
        //EventLoop就拿到了它的Poller给它返回的所有发生事件的channel列表了
    }
}

void EPollPoller::update(int operation,Channel * channel)
{
    epoll_event event;
    ::memset(&event,0,sizeof(event));
    int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl_del error :%d\n",errno);
        }else{
            LOG_FATAL("epoll_ctl add/mod error:%d\n",errno);
        }
    }
}