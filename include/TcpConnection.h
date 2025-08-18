#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;

/*
TcpServer => Acceptor => 有一个新用户的连接 通过acceptor函数拿到connfd
=> TcpConnection 设置回调 =》设置到Channel =》Poller => Channel回调
*/
class TcpConnection : noncopyable, public ::std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &nameArg,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() { return name_; }
    const InetAddress &localAddr() { return localAddr_; }
    const InetAddress &peerAddr() { return peerAddr_; }

    bool connected() const { return state_ == KConnected; }

    // 发送数据
    void send(const std::string &buf);
    void sendFile(int fileDescriptor, off_t offset, size_t count);

    // 关闭连接
    void shutdown();

    // 设置对应的回调函数
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    // 建立连接
    void connectEstablished();
    // 销毁连接
    void connectDestory();

private:
    enum StateE
    {
        kDisconnected, // 已经断开连接
        kConnecting,   // 正在连接
        KConnected,    // 已连接
        kDisconnecting // 正在断开连接
    };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();
    void sendFileInLoop(int fileDescriptor, off_t offset, size_t count);

    EventLoop *loop_; // 所属的loop,多Reactor下一定为subloop,不然就只能是mainloop

    const std::string name_;
    std::atomic_int state_; // 连接的状态，线程safe
    bool reading_;          // 连接是否在监听读事件

    // 这里和Acceptor类似 不过Acceptor只关系连接事件 Acceptor => mainloop  TcpConnection => subloop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_; // 对端地址

    // 这些回调由用户注册的给TcpServer,TcpServer传递给TCpConnection,TCpConnection再将回调注册到Channel中
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_; // 高水位阈值

    // 数据缓冲区
    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
};