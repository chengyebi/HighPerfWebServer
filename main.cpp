#include <iostream>
#include <vector>
#include <cstring> //bzero
#include <unistd.h> //read,write,close
#include <fcntl.h> //fcntl
#include <cerrno> //errno,EAGAIN
#include "Socket.h"
#include "Epoll.h"
#include "InetAddress.h"

#define BUFFER_SIZE 1024

//设置非阻塞模式
void setNonBlocking(int fd) {
    int flags=fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}

void handleReadEvent(int sockfd){
    char buf[BUFFER_SIZE];
    while (true) {
        bzero(&buf,sizeof(buf));
        ssize_t bytes_read=read(sockfd,buf,sizeof(buf));
        if (bytes_read>0) {
                //业务逻辑是回显数据
                std::cout<<"Message from client fd"<<sockfd<<":" <<buf;
                write(sockfd,buf,bytes_read);
        }
            else if (bytes_read==-1) {
                if (errno==EINTR) {
                    continue;//信号中断，继续尝试
                }
                if (errno==EAGAIN||errno==EWOULDBLOCK) {
                    break;//缓冲区已经空，必须等待下次通知
                }
                //真正的IO错误
                perror("read error");
                close(sockfd);
                break;
            }
            else if (bytes_read==0) {
                //客户端关闭连接
                std::cout<<"EOF,client fd"<<sockfd<<"disconnected"<<std::endl;
                close(sockfd);
                break;
            }
    }
}
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
    std::vector<struct epoll_event> events;
    std::cout<<"HighPerfWebServer running on port 8888……"<<std::endl;
    while (true) {
        //-1表示永久阻塞等待
        ep.poll(events,-1);
        int event_count=events.size();
        for (int i=0;i<event_count;i++) {
            int sockfd=events[i].data.fd;
            //处理新连接
            if (sockfd==serv_sock.getFd()) {
                InetAddress client_addr;
                int client_fd=serv_sock.accept(client_addr);
                if (client_fd!=-1) {
                    setNonBlocking(client_fd);
                    //客户端连接也采用ET模式
                    ep.addFd(client_fd,EPOLLIN|EPOLLET);
                    std::cout<<"New client connected :fd "<<client_fd<<std::endl;
                }
            }
            else if (events[i].events& EPOLLIN) {
                handleReadEvent(sockfd);
            }
            else {
                std::cout<<"something else happened"<<std::endl;
            }
        }
    }
    return 0;
}