#pragma once

#include "TcpServer.h"
#include "ProtocolParser.h"
#include "KeyRecommander.h"
#include "Configuration.h"
#include "Config.h"
#include <cstdint>

class SearchEngineServer
{
public:
    SearchEngineServer(const std::string &ip, short port);
    ~SearchEngineServer();
    void start();
    
    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp);
    void doTaskThread(const TcpConnectionPtr &conn, const std::string& msg); 
    void onClose();
    void onWriteComplete(const TcpConnectionPtr &conn);  // 写完成回调
private:
    TcpServer _tcpServer;
    EventLoop *_loop;
    ProtocolParser _protocolParser;  // 协议解析器
    //KeyRecommander _keyRecommander;
    //WebPageSearcher _webPageSearcher;
};
