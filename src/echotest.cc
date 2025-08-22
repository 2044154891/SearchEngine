#include <iostream>
#include <string>
#include <functional>
#include "TcpServer.h"

using namespace std;

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(loop), server_(loop, listenAddr, "EchoServer") {
        server_.setThreadNum(4);
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_.setWriteCompleteCallback(std::bind(&EchoServer::onWriteComplete, this, std::placeholders::_1));
    }
    void start() { server_.start(); }
private:
    void onConnection(const TcpConnectionPtr& conn) {
        // if (conn->connected()) {
        //     LOG_INFO("New connection UP: %s -> %s",
        //         conn->peerAddr().toIpPort().c_str(),
        //         conn->localAddr().toIpPort().c_str());
        // } else {
        //     LOG_INFO("Connection DOWN: %s -> %s",
        //         conn->peerAddr().toIpPort().c_str(),
        //         conn->localAddr().toIpPort().c_str());
        // }
    }
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg); // 回显
    }
    void onWriteComplete(const TcpConnectionPtr&) {
        // 可留空
    }
    EventLoop* loop_;
    TcpServer server_;
};

int main() {
    EventLoop loop;
    InetAddress listenAddr("127.0.0.1", 12345); // 监听12345端口
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
    return 0;
} 