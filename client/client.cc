// client/simple_test.cpp
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>

int main() {
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8888);
    serveraddr.sin_addr.s_addr = inet_addr("192.168.147.131");

    if (connect(clientfd, (const struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("connect");
        close(clientfd);
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;
    std::cout << "Enter messages to test (type 'quit' to exit):" << std::endl;

    std::string content;
    while (true) {
        std::cout << "\nEnter message: ";
        std::getline(std::cin, content);
        
        if (content == "quit" || content == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }
        
        if (content.empty()) {
            std::cout << "Message cannot be empty, please try again." << std::endl;
            continue;
        }

        // 发送测试消息
        uint32_t msgId = 1; // TASK_RECOMMEND_KEYWORDS
        uint32_t lenNet = htonl(content.size());
        uint32_t idNet = htonl(msgId);
        
        std::string frame;
        frame.resize(8 + content.size());
        std::memcpy(&frame[0], &lenNet, 4);
        std::memcpy(&frame[4], &idNet, 4);
        std::memcpy(&frame[8], content.data(), content.size());
        
        std::cout << "Sending: len=" << content.size() << ", id=" << msgId << ", content='" << content << "'" << std::endl;
        
        if (send(clientfd, frame.data(), frame.size(), 0) == -1) {
            perror("send failed");
            std::cout << "Connection lost, exiting..." << std::endl;
            break;
        }
        
        // 接收响应
        uint32_t respLen, respId;
        ssize_t recvResult = recv(clientfd, &respLen, 4, 0);
        if (recvResult <= 0) {
            perror("recv failed");
            std::cout << "Connection lost, exiting..." << std::endl;
            break;
        }
        
        recvResult = recv(clientfd, &respId, 4, 0);
        if (recvResult <= 0) {
            perror("recv failed");
            std::cout << "Connection lost, exiting..." << std::endl;
            break;
        }
        
        uint32_t bodyLen = ntohl(respLen);
        respId = ntohl(respId);
        
        std::cout << "Received: len=" << bodyLen << ", id=" << respId << std::endl;
        
        if (bodyLen > 0) {
            std::string respBody(bodyLen, '\0');
            recvResult = recv(clientfd, &respBody[0], bodyLen, 0);
            if (recvResult > 0) {
                std::cout << "Response: " << respBody << std::endl;
            } else {
                perror("recv body failed");
                std::cout << "Connection lost, exiting..." << std::endl;
                break;
            }
        }
    }
    
    close(clientfd);
    return 0;
}
