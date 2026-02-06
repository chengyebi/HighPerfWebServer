#include "Epoll.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>

//构造函数，创建epoll句柄
Epoll::Epoll():epfd_(-1),events_(1024) {
    //初始化预留1024个事件的空间
    //epoll_create1(0)比epoll_create更现代，0表示无特殊标志
    epfd_=epoll_create1(0);
    if (epfd_==-1) {
        throw std::runtime_error("epoll create error");
    }
}
//析构函数，关闭句柄
Epoll::~Epoll() {
    if (epfd_!=-1) {
        close(epfd_);
        epfd_=-1;
    }
}
//添加文件描述符
void Epoll::addFd(int fd,uint32_t op) {
    struct epoll_event ev;
    bzero(&ev,sizeof(ev));
    ev.data.fd=fd;
    ev.events=op;
    if (epoll_ctl(epfd_,EPOLL_CTL_ADD,fd,&ev)==-1) {
        throw std::runtime_error("epoll add event error");
    }
}
//等待事件
void  Epoll::poll(std::vector<struct epoll_event>& active_events,int timeout) {
    //调用epoll_wait
    //events_.data()返回的是vector底层数组的首地址
    //events_size()是maxevents;
    int nfds=epoll_wait(epfd_,events_.data(),
    static_cast<int>(events_.size()),timeout);
    if (nfds==-1) {
        if (errno==EINTR) {
            //收到信号中断，返回空集合，让上层继续循环;
            return;
        }
        throw std::runtime_error("epoll wait error");
    }
    //只填充，不创建
   active_events.clear();
    for (int i=0;i<nfds;++i) {
        active_events.push_back(events_[i]);
    }
}