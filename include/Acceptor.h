#pragma once 

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop * loop , const InetAddress & listenAddr, bool reuseport);
    ~Acceptor();
    //设置新连接的回调函数
    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }
    //判断是否存在监听
    bool listeing() const { return listening_; }
    //监听本地端口
    void listen();
private:
    void handleRead(); //处理用户的连接事件

    EventLoop * loop_; //Acceptor用的就是用户定义的baseloop 也叫mainReactor
    Socket acceptSocket_; //用于专门接收新连接的socket
    Channel acceptChannel_; //用于专门监听新连接的Channel
    NewConnectionCallback NewConnectionCallback_ ; //新连接的回调函数
    bool listening_; //是否存在监听
};