#include "InetAddress.h"
#include <stdexcept>
InetAddress::InetAddress() : addr{}, addr_len(sizeof(addr)) {
} //清空地址+初始化长度

InetAddress::InetAddress(const char *ip, uint16_t port) : addr{}, addr_len(sizeof(addr)) {
    addr.sin_family = AF_INET; //协议族设为IPv4
    if (inet_pton(AF_INET,ip,&addr.sin_addr)<=0) {//将点分十进制的IP地址字符串转换为网络字节序的二进制格式
        throw std::runtime_error("Invalid IP address");
    }//inet_pton返回0表示ip地址无效，返回-1表示协议族不支持，返回1表示转换成功

    addr.sin_port = htons(port); //主机字节序转换成网络字节序，网络传输统一必须用大端序
}

const struct sockaddr_in *InetAddress::getAddr() const{
    //返回地址
    return &addr;
}

socklen_t InetAddress::getAddrLen() const {
    //返回长度
    return addr_len;
}

void InetAddress::setAddr(struct sockaddr_in addr_in, socklen_t addr_len_in) {
    addr = addr_in;
    addr_len = addr_len_in;
}
