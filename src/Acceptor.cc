#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen sockfd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer::start() => Acceptor.listen() if有新的用户连接需要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    //baseloop 监听到有事件发生 =》 acceptChannel_(listenfd) => 执行该回调函数
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); //把Poller中感兴趣的事件删除
    acceptChannel_.remove(); //调用EventLoop->removeChannel => poller -> removeChannel 把Poller的ChannelMap对应的部分删除
}

void Acceptor::handleRead()// listenfd有事件发生了，就是有新用户连接了
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr); //轮询找到subLoop唤醒并分发当前新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept_ error:%d\n",__FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}


void Acceptor::listen()
{
    listening_ =true;
    acceptSocket_.listen(); //listen
    acceptChannel_.enableReading(); //acceptChannel_注册至Poller !重要
}

