#include "InetAddress.h"

//默认构造函数，初始化地址长度，并清空addr结构体
InetAddress::InetAddress() :  addr{},addr_len(sizeof(addr)) {
}

//带参构造函数，核心逻辑
InetAddress::InetAddress(const char* ip,uint16_t port):addr{},addr_len(sizeof(addr))  {
    //1.先清空整个结构体，防止有垃圾数据

    //2.设置协议族为IPv4
    addr.sin_family = AF_INET;

    //3.转换IP地址，从字符串“120.0.0.1”提取网络的整数
    //inet_addr会自动处理点分十进制
    addr.sin_addr.s_addr=inet_addr(ip);

    //4.转换端口号：从主机字节序转换成网络字节序（大端存储）
    //htons=host to Network Short
    addr.sin_port=htons(port);
}

//获取内部结构体的指针，给bind函数用的
struct sockaddr_in* InetAddress::getAddr() {
    return &addr;
}

//获取结构体的长度，给bind函数用的
socklen_t InetAddress::getAddrLen() const {
    return addr_len;
}