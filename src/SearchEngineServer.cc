#include "SearchEngineServer.h"

SearchEngineServer::SearchEngineServer(const std::string &ip, short port)
    : _loop(new EventLoop)
    , _tcpServer(_loop, InetAddress(ip, port), "SearchEngine")
{
    _tcpServer.setConnectionCallback(std::bind(&SearchEngineServer::onConnection, this, std::placeholders::_1));
    _tcpServer.setMessageCallback(std::bind(&SearchEngineServer::onMessage, this
        , std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _tcpServer.setWriteCompleteCallback(std::bind(&SearchEngineServer::onWriteComplete, this, std::placeholders::_1));
}

SearchEngineServer::~SearchEngineServer() {
}

void SearchEngineServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime) {
    // 使用协议解析器处理消息
    _protocolParser.parseAndDispatch(conn, buf);
}

void SearchEngineServer::onWriteComplete(const TcpConnectionPtr &conn) {
   
    std::cout << "Message sent to connection: " << conn->name() << std::endl;
}

void SearchEngineServer::onConnection(const TcpConnectionPtr &conn) {
    std::cout << "New connection: " << conn->name() << "\n";
}

void SearchEngineServer::onClose() {
    std::cout << "Server closed" << "\n";
}

void SearchEngineServer::doTaskThread(const TcpConnectionPtr &conn, const std::string& msg) {
    // 这个方法可以保留用于其他用途，或者删除
}

void SearchEngineServer::start() {
    _tcpServer.start();
    // 启动事件循环，否则服务器不会处理任何事件
    if (_loop) {
        _loop->loop();
    }
}

int main() {
    // 读取配置
    Configuration* cfg = Configuration::getInstance(Config::configFilePath);
    auto &mp = cfg->getConfigMap();
    // 从配置中读取服务器地址与端口，若不存在则使用默认值
    std::string ip = mp.count("server_ip") ? mp["server_ip"] : std::string("127.0.0.1");
    short port = mp.count("server_port") ? static_cast<short>(std::stoi(mp["server_port"])) : static_cast<short>(8888);

    SearchEngineServer server(ip, port);
    server.start();
    return 0;
}