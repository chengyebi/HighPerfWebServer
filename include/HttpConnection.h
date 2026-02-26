#pragma once
#include "Socket.h"
#include "Buffer.h"
#include "HttpRequest.h"
#include "Epoll.h"
#include <memory>
#include <string>

class HttpConnection {
public:
    HttpConnection(int fd);
    ~HttpConnection();

    //禁止拷贝，独占资源
    HttpConnection(const HttpConnection&)=delete;
    HttpConnection& operator=(const HttpConnection&)=delete;
    int getFd() const;
    void handleRead(Epoll& ep);
    void handleWrite(Epoll& ep);
    bool isClosed() const;
private:
    void process();
    void closeConnection();

    Socket socket_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    HttpRequest request_;
    bool keepAlive_;
    //零拷贝文件传输相关的
    int fileFd_;
    off_t fileOffset_;//当前发送到的偏移量
    size_t fileLen_;//文件总长度

};