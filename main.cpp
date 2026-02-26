#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Socket.h"
#include "Epoll.h"
#include "InetAddress.h"
#include "HttpConnection.h"

int main() {
    //建立监听，
    Socket serv_sock;
    InetAddress serv_addr("127.0.0.1",8888);
    serv_sock.bind(serv_addr);
    serv_sock.listen();
    //初始化epoll
    Epoll ep;
    serv_sock.setNonBlocking();
    //监听Socket用ET模式，达到高性能处理
    ep.addFd(serv_sock.getFd(),EPOLLIN|EPOLLET);
    //连接池，映射
    std::unordered_map<int,std::shared_ptr<HttpConnection>> connections;
    std::cout<<"HighPerfWebServer running on port 8888……"<<std::endl;
    while (true) {
        std::vector<epoll_event> active_events;
        ep.poll(active_events);
        for (auto& event: active_events) {
            int fd=event.data.fd;
            //场景A:有新的客户端连接
            if (fd==serv_sock.getFd()) {
                //ET模式必须循环accept
                while (true) {
                    InetAddress client_addr;
                    int client_fd=serv_sock.accept(client_addr);
                    if (client_fd==-1) {
                        if (errno==EAGAIN||errno==EWOULDBLOCK) {//全连接队列里等待的客户端已经被拿光,正常结束
                            break;
                        }
                        else break;//发生错误，异常，强制结束，这个地方没有直接在fd==-1时直接写break而是写了两个分支，是为了为后续的扩展功能占下空位
                    }
                    //创建连接对象并存入哈希表，智能指针管理生命周期
                    connections[client_fd]=std::make_shared<HttpConnection>(client_fd);
                    //注册客户端socket的可读事件
                    ep.addFd(client_fd,EPOLLIN|EPOLLET| EPOLLONESHOT);
                }
            }
            //场景B：已有客户端发来数据或可写
            else {
                if (connections.count(fd)==0)
                    continue;
                auto conn=connections[fd];
                if (event.events&EPOLLIN) {
                    conn->handleRead(ep);
                }
                if (event.events&EPOLLOUT) {
                    conn->handleWrite(ep);
                }
                //收尾：如果连接被标记为关闭，将其从map中剔除
                //此时shared_ptr引用计数为0，HttpConnection自动析构，安全释放fd
                if (conn->isClosed()) {
                    connections.erase(fd);
                }
            }
        }


    }
    return 0;
}