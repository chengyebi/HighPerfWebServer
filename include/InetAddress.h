#pragma once
#include <arpa/inet.h>

class InetAddress {
    //网络套接字类
private:
    struct sockaddr_in addr; //网络套接字的地址结构体：协议族、ip+port
    socklen_t addr_len; //网络套接字的地址结构体长度
public:
    InetAddress(); //默认构造函数
    InetAddress(const char *ip, uint16_t port); //带参构造函数
    ~InetAddress() = default; //析构函数用默认的
    [[nodiscard]] const struct sockaddr_in *getAddr() const; //得到地址,[[nodiscard]]编译时提醒不忽略返回值
    socklen_t getAddrLen() const; //得到长度
    void setAddr(struct sockaddr_in addr_in, socklen_t addr_len_in);
};
