#include "Buffer.h"
#include <cstring>
#include <unistd.h>//write
#include <sys/uio.h>//readv
#include <errno.h>
//构造函数
Buffer::Buffer(int initBuffSize):buffer_(initBuffSize),readPos_(0),writePos_(0){}
//可读字节数等于写指针-读指针
size_t Buffer::readableBytes() const {
    return writePos_-readPos_;
}

size_t Buffer::writableBytes() const {
    return buffer_.size()-writePos_;
}

//头部预留空间
size_t Buffer::prependableBytes() const {
    return readPos_;
}
//返回当前读指针的地址
const char* Buffer::peek() const {
    return beginPtr_()+readPos_;
}
//读走了len字节，移动读指针
void Buffer::retrieve(size_t len) {
    if (len<readableBytes())
        readPos_+=len;//还没读完
    else {
        retrieveAll();//读完了，重置指针
    }
}
void Buffer::retrieveUntil(const char* end) {
    if (end>=peek()) {//需要移动到的位置超过了当前读指针的位置，需要移动
    retrieve(end-peek());//参数传的是读指针移动的长度
    }
}
//读写指针归零
void Buffer::retrieveAll() {
    bzero(&buffer_[0],buffer_.size());
    readPos_=0;
    writePos_=0;
}

std::string Buffer::retrieveAllAsString() {
    std::string str(peek(),readableBytes());//从当前读指针开始一直把所有可读数据转换成字符串
    retrieveAll();
    return str;
}

const char* Buffer::beginPtr_() const {
    return &*buffer_.begin();
}
char* Buffer::beginPtr_() {
    return &*buffer_.begin();
}
//确保能写len字节，不够就继续扩容
void Buffer::ensureWriteable(size_t len) {
   if (writableBytes()<len) {
       makeSpace_(len);
   }
}
void Buffer::append(const std::string& str) {
    append(str.data(),str.length());
}

void Buffer::append(const void* data,size_t len) {
    if (data == nullptr) return;
    append(static_cast<const char*>(data),len);
}

//核心追加函数
void Buffer::append(const char* str,size_t len) {
    ensureWriteable(len);
    std::copy(str,str+len,beginPtr_()+writePos_);
    hasWritten(len);
}
void Buffer::hasWritten(size_t len) {
    writePos_+=len;
}
//空间管理
void Buffer::makeSpace_(size_t len) {
    //剩余空间+头部已读空间<需要空间，则必须扩容了
    if (writableBytes()+prependableBytes()<len) {
        buffer_.resize(writePos_+len+1);
    }
    else {
        //空间够，不扩容也可以，只是需要把中间的数据移到前面，把前面已读的空间利用了
        size_t readable=readableBytes();
        std::copy(beginPtr_()+readPos_,beginPtr_()+writePos_,beginPtr_());
        readPos_=0;
        writePos_=readPos_+readable;
    }
}
//readv,一种实现分散读的系统调用
//普通的read系统调用，一次只能把数据读进一个缓冲区，但是分散读可以一次系统调用读进多个缓冲区，
//这在一个缓冲区空间不够的时候，能够把剩下的数据直接读到另一个缓冲区，减少了内核态的切换开销
//buffer空间不够的时候就读进栈上临时数组extraBuff,数据少直接读进buffer，数据多，就先填满buffer,再填满extraBuff
ssize_t Buffer::readFd(int fd,int* Errno) {
    char extraBuff[65535];//栈上临时分配64K空间
    struct iovec iov[2];//IO向量
    const size_t writable=writableBytes();
    //第一步，指向buffer内部的剩余可写空间
    iov[0].iov_base=beginPtr_()+writePos_;
    iov[0].iov_len=writable;
    //第二块，指向栈上的临时空间
    iov[1].iov_base=extraBuff;
    iov[1].iov_len=sizeof(extraBuff);
    const size_t iov_len=sizeof(extraBuff);
    const size_t iovcnt=(writable<sizeof(extraBuff)?2:1);
    const ssize_t len=readv(fd,iov,iovcnt);
    if (len<0) {
        *Errno=errno;
    }
    else if (static_cast<size_t>(len) <=writable) {
        writePos_+=len;
    }
    else {
        writePos_=buffer_.size();
        //把extraBuff的数据追加到buffer里，buffer要扩容
        append(extraBuff,len-writable);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd,int* Errno) {
    size_t readSize=readableBytes();
    ssize_t len=write(fd,peek(),readSize);
    if (len<0) {
        *Errno=errno;
        return len;
    }
    readPos_+=len;
    return len;
}