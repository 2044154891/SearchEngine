#pragma once

#include "TcpServer.h"
#include "ProtocolParser.h"

class SearchEngineServer
{
public:
    SearchEngineServer(const std::string &ip, short port);
    ~SearchEngineServer();
    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp);
    void doTaskThread(const TcpConnectionPtr &conn, const string& msg); 
    void start();
    void onClose();
    void onWriteComplete(const TcpConnectionPtr &conn);  // 写完成回调
private:
    TcpServer _tcpServer;
    EventLoop *_loop;
    ProtocolParser _protocolParser;  // 协议解析器
};
