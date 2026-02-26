#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>  //perror
#include <iostream>
#include <vector>
#include <atomic>
#include <unistd.h>//read write
#include <sys/uio.h>//readv
class Buffer {
public:
    //构造函数，默认分配，1024字节
    explicit Buffer(int initBuffSize=1024);
    ~Buffer()=default;
    //核心状态查询，还有多少数据可写？
    size_t writableBytes() const;
    //还有多少数据能读：
    size_t readableBytes() const;
    //读指针前面空出多少，可用于腾挪空间
    size_t prependableBytes() const;
    //数据读取接口
    //不移动指针，只是看看现在指针的位置
    const char* peek() const;
    //读完数据后，要把读指针往后移，
    void retrieve(size_t len);
    //移动到指定位置
    void retrieveUntil(const char* end);
    //把所有数据都读完清空缓冲区
    void retrieveAll();
    //把所有数据读完变成string返回
    std::string retrieveAllAsString();

    //数据写入接口
    //保证有足够的空间写len字节
    void ensureWriteable(size_t len);
    //写完数据后移动写指针
    void hasWritten(size_t len);

    //往buffer里塞进数据
    void append(const std::string& str);
    void append(const char* str,size_t len);
    void append(const void* data,size_t len);
    void append(const Buffer& buff);

    //核心IO接口
    //从文件描述符fd读数据到buffer,处理ET模式下的读
    ssize_t readFd(int fd,int* Errno);
    //把buffer数据写到fd
    ssize_t writeFd(int fd,int* Errno);

private:
    //获取buffer内部vector的起始地址
    char* beginPtr_();
    const char* beginPtr_() const;
    //扩容或整理空间
    void makeSpace_(size_t len);
    //真正的容器：用vector管理char,自动处理内存释放，RAII
    std::vector<char> buffer_;
    //读位置
    std::atomic<std::size_t> readPos_;
    //写位置
    std::atomic<std::size_t> writePos_;
};
#endif //BUFFER_H