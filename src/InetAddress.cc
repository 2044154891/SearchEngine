#include <strings.h>
#include <string.h>

#include "InetAddress.h" 

InetAddress::InetAddress(std::string ip, uint16_t port)
{
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET; //地址簇
    addr_.sin_port = ::htons(port);
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str()); //点分十进制IP地址转化为网络字节序的2进制
}

std::string InetAddress::toIp() const
{
    // addr_
    char buff[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buff, sizeof(buff));
    return buff;
}
std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}


// #include <iostream>
// // int main(){
// //     InetAddress  addr(8080);
// //     std::cout << addr.toIpPort() << std::endl;
// // }
