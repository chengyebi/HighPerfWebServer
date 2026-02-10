#pragma once
class InetAddress;//在此声明，在cpp里#include
//封装socket文件描述符，管理生命周期，Resource acquisition is initialization，RAII

class Socket {
private:
    int fd_;//socket文件描述符，是唯一的资源
public:
    //1.默认构造，创建一个IPv4 TCP socket
    Socket();
    //2.包装构造：传入一个已有的fd,用于accept返回的客户端fd;
    explicit Socket(int fd);
    //3.析构：对象销毁时自动closed(fd),防止资源泄露
    ~Socket();
    //禁止拷贝
    //Socket独占fd资源，如果允许拷贝，会导致两个对象析构时double close错误
    Socket(const Socket&) = delete;//显式删除函数，表示这个函数不能被调用，这里指禁止拷贝构造，不能实现=delete的函数
    Socket& operator =(const Socket&) =delete;//这里禁止拷贝赋值
    //允许移动，socket资源可以转移
    //加上noexcept,让vector扩容时可以用移动构造
    Socket(Socket&& other) noexcept;//noexcept指定函数不会抛出异常
    Socket& operator=(Socket&& other) noexcept;
    //核心网络操作
    void bind(const InetAddress& addr);//绑定地址
    void listen();//开始监听
    [[nodiscard]] int accept(InetAddress& client_addr);//nodiscard是c++17特性，忽略返回值会导致编译器警告
    void setNonBlocking();//设置非阻塞，Epoll ET模式需要这个
    int getFd() const;//获取fd;
};