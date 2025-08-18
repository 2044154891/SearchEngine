#include "SearchEngineServer.h"

SearchEngineServer::SearchEngineServer(const std::string &ip, short port)
    : _tcpServer(ip, port)
{
    _tcpServer.setConnectionCallback(std::bind(&SearchEngineServer::onConnection, this, std::placeholders::_1));
    _tcpServer.setMessageCallback(std::bind(&SearchEngineServer::onMessage, this
        , std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _tcpServer.setWriteCompleteCallback(std::bind(&SearchEngineServer::onWriteComplete, this, std::placeholders::_1));
    _tcpServer.setCloseCallback(std::bind(&SearchEngineServer::onClose, this));
}

SearchEngineServer::~SearchEngineServer() {
}

void SearchEngineServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime) {
    // 使用协议解析器处理消息
    _protocolParser.parseAndDispatch(conn, buf);
}

void SearchEngineServer::onWriteComplete(const TcpConnectionPtr &conn) {
    // 消息发送完成后的处理逻辑
    // 例如：记录日志、统计信息等
    std::cout << "Message sent to connection: " << conn->name() << std::endl;
}

void SearchEngineServer::onConnection(const TcpConnectionPtr &conn) {
    std::cout << "New connection: " << conn->name() << std::endl;
}

void SearchEngineServer::onClose() {
    std::cout << "Server closed" << std::endl;
}

void SearchEngineServer::doTaskThread(const TcpConnectionPtr &conn, const string& msg) {
    // 这个方法可以保留用于其他用途，或者删除
}

void SearchEngineServer::start() {
    _tcpServer.start();
}