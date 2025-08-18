#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h>  //for open
#include <unistd.h> //for close

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

// 防止重复定义，事件循环正确性检查
static EventLoop *CheckLoopNotNUll(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d belonging loop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNUll(loop)), name_(nameArg), state_(kConnecting), reading_(true), 
    socket_(new Socket(sockfd)), channel_(new Channel(loop_, sockfd)), 
    localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 下面给channel设置相应的回调函数 Poller给Channel通知感兴趣的事件发送 Channel会回调相应的函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == KConnected)
    {
        if (loop_->isInLoopThread()) // 这种是对于单个reactor的情况 用户调用conn-send时 loop_即为当前线程
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

/*发送数据 应用写的快 但是内核发送数据慢 需要把带发送数据写入缓冲区，设置了水位回调
 */
void TcpConnection::sendInLoop( const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if(state_ == kDisconnected) //之前调用过tcpconnecion 的shutdown不能再进行send
    {
        LOG_ERROR("disconnectied, give up writing");
    }
    //表示channel_第一次start写数据或者缓冲区没有发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote > 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                //数据发送全部完成 不再给channel设置epollout事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }
        else//nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) //EWOULDBLOCK 表示没有阻塞下没有数据后的正常return 等同于EAGAIN
            {
                LOG_ERROR("Tcpconnection::sendInloop");
                if(errno == EPIPE || errno == ECONNRESET) //SIGPIPI RSET
                {
                    faultError = true;
                }
            }
        }
    }
    /*
    说明当前这一次write没有把数据全部send出去，剩余的数据保存到缓冲区当中
    给channel注册EPOLLOUT事件，POller发现tcp的send缓冲区有space会通知相应的
    channel,调用channel注册的writecallback()
    channel的writeCallback()其实就是TCpconnection设置的handleWrite回调
    把send缓冲区outputBuffer的数据全部发送完成
    */
   if(!faultError && remaining > 0)
   {
        //目前send缓冲区剩余的待发送的数据长度
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting(); //这里一定要注册channel的写事件，否则poller不会给channel通知epollout
        }
   }
}

//建立连接
void TcpConnection::connectEstablished()
{
    setState(KConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); //向poller注册channel的EPOLLIN读事件

    //新连接建立 执行回调
    connectionCallback_(shared_from_this());
}

//销毁连接
void TcpConnection::connectDestory()
{
    if(state_ == KConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); //把channel所有感兴趣的事件从Poller中删除
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); //把channel从poller中删除
}

 //读是相对于服务器而言的 当客户端有数据到达时 服务器端的EPOLLIN 就会触发fd上的回调 handleread读取对端发来的数据
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0) //有数据到达
    {
        //已经建立连接的用户由可读事件发送了 调用用户传入的回调操作onMessage shared_from_this 就是获取了TcpConnection的智能指针
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0) //客户端断开
    {
        handleClose();
    }
    else //出错了
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if( n > 0)
        {
            outputBuffer_.retrieve(n); //从缓冲区读取readble区域数据移动到readindex下标
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    //TcpConnection对象在其所在的subloop中 向pendingFunctors_加入回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop(); //在当前所属的loop中把tcpConnection删除调
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=$%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);  //连接回调
    closeCallback_(connPtr);  //执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法 //must be the last line
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) //套接字状态获取失败
    {
        err = errno; //当前系统的错误码
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}

void TcpConnection::shutdown()
{
    if(state_ == kDisconnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

//关闭写端
void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) //说明当前outputBuffer_的数据全部向外发送完毕
    {
        socket_->shutdownWrite();
    }
}

//新增的零拷贝函数
void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t count)
{
    if(connected())
    {
        if(loop_->isInLoopThread())
        {
            sendFileInLoop(fileDescriptor, offset, count);
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, count));
        }
    }
    else
    {
        LOG_ERROR("TcpConnection::sendFile -- not connected");
    }
}

//在事件循环中执行sendfile
void TcpConnection::sendFileInLoop(int fileDescriptor, off_t offset, size_t count)
{
    ssize_t bytesSent = 0; //发送了多少字节数
    size_t remaining = count; //还要多少数据要发送
    bool faultError = false; //错误的标志位

    if(state_ == kDisconnecting){ //表示此时连接已经断开就不需要再发送数据了
        LOG_ERROR("disconnected, give up writing");
        return;
    }
    //表示Channel第一次开始写数据或者outputBuffer缓冲区中没有数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        bytesSent = sendfile(socket_->fd(), fileDescriptor, &offset, remaining);
        if(bytesSent >= 0)
        {
            remaining -= bytesSent;
            if(remaining == 0 && writeCompleteCallback_)
            {
                //remaining为0意味着数据正好全部发送完成，不需要给其设置写事件的监听
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            //bytesSent < 0
            if(errno != EWOULDBLOCK)
            {
                //如果是非阻塞没有数据返回错误这个是正常现象等同于EAGAIN,否则就是异常情况
                LOG_ERROR("Tcpconnection::sendfileInLoop");
            }
            if(errno == EPIPE || errno == ECONNRESET)
            {
                faultError = true;
            }
        }
    }
    //处理剩余数据
    if(!faultError && remaining > 0)
    {
        //继续发送剩余数据
        loop_->queueInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, remaining));
    }
}