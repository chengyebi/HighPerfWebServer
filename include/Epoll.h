#pragma once
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

class Epoll {
private:
    int epfd_;//epoll实例的文件描述符
    //用于接收epoll_wait返回的就绪事件
    //使用vector可以在扩容时动态管理内存，，比数组安全
    std::vector<struct epoll_event> events_;
public:
    Epoll();
    ~Epoll();
    //禁止拷贝，epfd_是独占资源
    Epoll(const Epoll&)=delete;
    Epoll& operator=(const Epoll&)=delete;
    //核心接口：添加文件描述符到epoll监控中
    //enable_et:是否开启ET模式，默认开启，开启是高性能
    void addFd(int fd,uint32_t op);
    //让epoll类有动态修改监听事件的能力
    void modFd(int fd,uint32_t events);
    //核心接口：等待事件发生
    //time_out:超时时间，-1表示永久阻塞，
    //返回活跃事件的组合（这里也可以返回int,然后提供getEvents(),但是直接填充到vector更方便；
    void poll(std::vector<struct epoll_event>& active_events,int timeout=-1);
};