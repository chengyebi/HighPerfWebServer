#include "InetAddress.h"

InetAddress::InetAddress() : addr{}, addr_len(sizeof(addr)) {
} //清空地址+初始化长度

InetAddress::InetAddress(const char *ip, uint16_t port) : addr{}, addr_len(sizeof(addr)) {
    addr.sin_family = AF_INET; //协议族设为IPv4
    addr.sin_addr.s_addr = inet_addr(ip); //点分十进制自动转换成32位二进制数
    addr.sin_port = htons(port); //主机字节序转换成网络字节序，网络传输统一必须用大端序
}

struct sockaddr_in *InetAddress::getAddr() {
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
