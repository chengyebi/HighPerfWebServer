#include "Socket.h"
#include "InetAddress.h"//accept的实现用到了其中的函数
#include <unistd.h>//要用它的close
#include <fcntl.h>//用它的fcntl
#include <sys/socket.h> //用它的socket,bind,listen等
#include <stdexcept>//用到了runtime_error
#include <cerrno> // errno

//默认构造函数
Socket::Socket():fd_(-1) {
    //创建IPv4,TCP流式套接字
    fd_=socket(AF_INET,SOCK_STREAM,0);
    if (fd_==-1) {
        throw std::runtime_error("socket create error");
    }
}

//包装构造函数
Socket::Socket(int fd):fd_(fd) {
        if (fd_==-1) {
            throw std::runtime_error("socket error:invalid fd");
        }
}

//析构函数，RAII核心
Socket::~Socket() {
    if (fd_!=-1) {
        close(fd_);//自动关闭，防止fd泄露
        fd_=-1;
    }
}

//绑定地址
void Socket::bind(const InetAddress& addr) {
    int opt=1;//设置端口复用，一个端口被释放后必须被“占用一定时间”才能被别人用，但是高并发需要立即用，所以设置可以申请立即复用，即reuse
    setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //调用系统bind
    //addr.getAddr()返回的是struct sockaddr_in*,需要强制转换
    int ret=::bind(fd_,(struct sockaddr*)addr.getAddr(),addr.getAddrLen());
    if (ret==-1) {//0代表成功，-1代表绑定失败
        throw std::runtime_error("socket bind error");
    }
}

//开始监听
void Socket::listen() {
    //系统建议的最大监听队列长度是SOMAXCONN
    int ret=::listen(fd_,SOMAXCONN);
    if (ret==-1) {
        throw std::runtime_error("socket listen error");
    }
}

//接受连接
int Socket::accept(InetAddress& client_addr) {
    struct sockaddr_in addr;
    socklen_t len=sizeof(addr);
    //accept支持阻塞（fd是阻塞的，没连接时线程卡在这里）和非阻塞（fd是非阻塞的，没连接时直接返回-1）
    int client_fd=::accept(fd_,(struct sockaddr*)&addr,&len);
    if (client_fd!=-1) {
        //连接上了！把客户端的IP,Port都填回去
        client_addr.setAddr(addr,len);
    }
    return client_fd;
}

//设置非阻塞，epoll et模式需要
void Socket::setNonBlocking() {
    int flags=fcntl(fd_,F_GETFL);//获取旧的标志
    fcntl(fd_,F_SETFL,flags | O_NONBLOCK);
}

//获取fd
int Socket::getFd() const {
    return fd_;
}

//移动语义的实现，
//移动构造：抢资源
Socket::Socket(Socket&& other) noexcept:fd_(other.fd_) {
    other.fd_=-1;//源对象的fd置为无效，防止它析构时关闭fd
}

//资源转让：移动赋值，先扔旧的，再抢新的
Socket& Socket::operator=(Socket&& other) noexcept {
    if (this!=&other) {
        if (fd_!=-1) {
            close(fd_);//自己手里如果有资源就先释放
        }
        fd_=other.fd_;//抢别人的资源
        other.fd_=-1;//释放别人的资源
    }
    return *this;
}
























