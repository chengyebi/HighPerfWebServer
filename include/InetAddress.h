#pragma once//防止头文件被重复包含
#include <arpa/inet.h>//包含socket核心结构体定义的头文件

//封装ip地址和端口号的类
class InetAddress {
private:

    //核心数据：这是linux内核认识的地址结构体（C风格）
    struct sockaddr_in addr;

    //地址结构体的长度，bind函数需要用到
    socklen_t addr_len;

public:
    //构造函数，传入ip（字符串）和端口（整数）
    InetAddress(const char* ip,uint16_t port);

    //默认构造函数，不做初始化
    InetAddress();

    //析构函数
    ~InetAddress()=default;

    //接口：把内部私有成员抛出去给别人使用
    struct sockaddr_in* getAddr();
    socklen_t getAddrLen() const;
};

/*📂 class InetAddress
 ┣ 🔒 private members
 ┃ ┣ 📦 addr           (就是上面的 struct sockaddr_in)
 ┃ ┗ 📏 addr_len       (记录 sizeof(addr))
 ┃
 ┗ 🔓 public methods
   ┣ 🔨 构造函数        (负责填写 addr 里的 family, port, s_addr)
   ┣ 📤 getAddr()      (return &addr;  --> 把底层结构体暴露给 bind 用)
   ┗ 📏 getAddrLen()   (return addr_len;)*/

/*📦 struct sockaddr_in  <-- 【一级容器】整个 IPv4 地址包
 ┣ 🏷️ sin_family       (2字节，固定填 AF_INET)
 ┣ 🏷️ sin_port         (2字节，网络字节序的端口号)
 ┃
 ┗ 📦 sin_addr         <-- 【二级容器】为了兼容性设计的中间层结构体
   ┗ 🔢 s_addr         <-- 【核心原子】真正的 32 位 IP 整数 (网络字节序)*/

/*📂 class Socket
 ┣ 🔒 private members
 ┃ ┗ 🔢 fd_            (Socket 文件描述符，系统的"句柄")
 ┃
 ┗ 🔓 public methods
   ┣ 🔨 构造函数        (fd_ = socket(...))
   ┣ ♻️ 析构函数        (close(fd_))
   ┃
   ┣ 🔗 Bind(InetAddress* addr)
   ┃  ┗ 内部逻辑：调用系统 ::bind(fd_, addr->getAddr(), ...)
   ┃    (解释：Socket 拿着 fd，去读取 InetAddress 里的 addr 数据)
   ┃
   ┗ 📞 Accept(InetAddress* addr)
      ┗ 内部逻辑：调用系统 ::accept(fd_, ...)
        (解释：有新连接时，内核把对方的 IP/Port 填入这个 addr 里)*/