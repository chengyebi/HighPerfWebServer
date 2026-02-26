#include "HttpConnection.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <sys/sendfile.h>
HttpConnection::HttpConnection(int fd)
:socket_(fd)
,keepAlive_(false)//默认短连接
,fileFd_(-1)
,fileOffset_(0)//文件发送偏移量清零
,fileLen_(0)
{
   socket_.setNonBlocking();//ET模式必须搭配非阻塞
}
HttpConnection::~HttpConnection() {//析构时确保打开的文件资源被释放
   if (fileFd_!=-1) {
      close(fileFd_);
      fileFd_=-1;
   }
}
int HttpConnection::getFd() const {
   return socket_.getFd();
}
bool HttpConnection::isClosed() const {
   return socket_.getFd() ==-1;
}
void HttpConnection::closeConnection() {
   socket_.close();//将fd_置为-1
}
//IO与业务，ET模式下的非阻塞读取
void HttpConnection::handleRead(Epoll& ep) {
   int saveErrno=0;
   ssize_t len=0;
   //ET模式必须循环读取，只在状态变化时通知一次，必须读到返回-1且errno为EAGAIN或EWOULDBLOCK，表示内核缓冲区已经被抽干
   while (true) {
      len=inputBuffer_.readFd(socket_.getFd(),&saveErrno);
      if (len<=0) {
         if (saveErrno==EAGAIN||saveErrno==EWOULDBLOCK) {
            break;//数据读没有了，直接退出循环
         }
         if (saveErrno==EINTR) {
            continue;//信号中断，继续读取
         }
         //不是上述情况就说明是真正的错误，这个时候就要关闭连接
         closeConnection();
         return;
      }
   }
   //这里的思路是先把所有数据一次性读取到Buffer然后再统一进行解析
   //主要是为了防止大文件或多个文件多次到达时，解析逻辑出现错误
   process();

   //解析完毕，若有数据要发，注册EPOLLOUT,这里同时保留了EPOLLIN!防止在发送大文件期间，客户端断开连接法FIN导致服务器无法连接
   //从而继续向已经关闭的socke填写数据，触发SIGPIPE导致进程崩溃
   if (outputBuffer_.readableBytes()>0||fileFd_!=-1) {
      handleWrite(ep);
   }
   else {
      // 如果没数据要发，说明是个“半包”，请求不完整
      // 必须重新挂载 EPOLLIN，让内核等下半个包到了再通知
      ep.modFd(socket_.getFd(), EPOLLIN | EPOLLET | EPOLLONESHOT);
   }
}
//ET模式下的非阻塞写
void HttpConnection::handleWrite(Epoll& ep) {
    ssize_t len=0;
   int saveErrno=0;
   while (true) {
      //发送buffer里的数据
   if (outputBuffer_.readableBytes()>0) {
      while (outputBuffer_.readableBytes()>0) {
         len=outputBuffer_.writeFd(socket_.getFd(),&saveErrno);
         if (len<=0) {
            if (saveErrno==EAGAIN||saveErrno==EWOULDBLOCK) {
               ep.modFd(socket_.getFd(),EPOLLIN|EPOLLOUT|EPOLLET|EPOLLONESHOT);
               return;
            }
            if (saveErrno==EINTR) {
               continue;//信号中断，重试
            }
            closeConnection();//真实错误，直接关闭连接
            return;
         }
      }
   }
   //发送大文件，真正的内核态零拷贝，有响应头发送完毕且有发送文件才执行
   if (outputBuffer_.readableBytes()==0&&fileFd_!=-1) {
      while (true) {
         ssize_t sent=sendfile(socket_.getFd(),fileFd_,&fileOffset_,fileLen_-fileOffset_);
         if(sent>0) {
             //发送了一部分
            if (fileOffset_>=(off_t)fileLen_) {
               //文件彻底发完了，清理资源
               close(fileFd_);
               fileFd_=-1;
               fileLen_=0;//状态清零，防止下次误判
               fileOffset_=0;
               break;
            }
            //没发完，继续循环尝试发送，直到填满发送缓冲区触发EAGAIN
         }
         else if (sent==-1) {
            if (errno==EAGAIN||errno==EWOULDBLOCK) {
               ep.modFd(socket_.getFd(),EPOLLIN|EPOLLOUT|EPOLLET| EPOLLONESHOT);
               return;//发送缓冲区满了，等待下次EPOLLOUT
            }
            if (errno==EINTR) {
               continue;//被信号中断，继续发送
            }
            //真实报错
            close(fileFd_);
            fileFd_=-1;
            fileLen_=0;
            closeConnection();
            return;
         }
         else {
            //sent=0,通常是对端关闭，这个时候要进行异常处理
            close(fileFd_);
            fileFd_=-1;
            fileLen_=0;
            closeConnection();
            return;
         }
      }
   }
   //收尾工作;
   //所有数据全部发完
   if (outputBuffer_.readableBytes()==0&&fileFd_==-1) {
      if (keepAlive_) {
      //重置解析器状态，准备处理下一个请求
         request_.init();
         //ET模式下的粘包处理
         /*客户端pipeline模式一次tcp发了两个请求，读事件只触发一次，第一次process只处理了第一个请求，
          * 当第一个请求处理完毕，若buffer里还有数据第二个（请求）此时内核不会再触发EPOLLIN,因为数据早就读上来了
          * 必须主动检查并处理残留数据，否则连接会假死
          */
         if (inputBuffer_.readableBytes()>0) {
            process();//立即处理滞留的下一个请求
            if (outputBuffer_.readableBytes()>0||fileFd_!=-1) {
               continue;
            }
         }
         //彻底空闲，安心等待下一次读事件
         ep.modFd(socket_.getFd(),EPOLLIN|EPOLLET| EPOLLONESHOT);
         return;
      }
      else {
         //短连接，发完就关
         closeConnection();
         return;
      }
   }
   }
}
//业务处理逻辑
void HttpConnection::process() {
   //尝试解析请求
   if (!request_.parse(inputBuffer_)) {
      //解析格式错误，如乱码，回复400并断开
      std::string body="<h1>400 Bad Request</h1>";
      outputBuffer_.append("HTTP/1.1 400 Bad Request\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body);
      keepAlive_=false;
      return;
   }
   //处理TCP拆包，parse成功不代表请求完整，如果只收到半个请求头，parse返回true但是isComplete为false
   //此时不能处理业务，必须等待后续数据到达
   if (!request_.isComplete()) {
      return;
   }
   //业务逻辑：构建响应
   keepAlive_ = true;
   std::string path=request_.path();
   //生成 Connection 头部字符串 ---
   std::string connStr = keepAlive_ ? "Connection: keep-alive\r\n" : "Connection: close\r\n";
   //因为..在Linux里是"上一级目录"，叠加足够多层就能逃到根目录，访问任意系统文件
   //为安全起见，加一个简单的防御目录攻击的设计
   //方法就是对于访问的目录检查是否存在..，存在就直接禁止访问
   if (path.find("..")!=std::string::npos) {
      std::string body="<h1>403 Forbidden</h1>";
      outputBuffer_.append("HTTP/1.1 403 Forbidden\r\n"+ connStr+"Content-Length: "+std::to_string(body.size())+"\r\n\r\n"+body);
      return;
   }
   std::string filePath="./resources"+path;
   struct stat fileStat;
   //查找文件
   if (stat(filePath.c_str(),&fileStat)<0) {
      //文件不存在，返回404
      std::string body="<h1>404 Not Found</h1>";
      outputBuffer_.append("HTTP/1.1 404 Not Found\r\n"+ connStr+"Content-Length: "+std::to_string(body.size())+"\r\n\r\n"+body);
   }
   else {
      // 1. 先尝试打开文件
      fileFd_ = open(filePath.c_str(), O_RDONLY);

      if (fileFd_ < 0) {
         // 打开失败,大概率是并发太高，文件描述符耗尽
         // 返回 500 错误
         std::string body = "<h1>500 Internal Server Error</h1>";
         outputBuffer_.append("HTTP/1.1 500 Internal Server Error\r\n" + connStr + "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body);
      } else {
         // 文件打开了，这时可以给客户端发 200 OK
         std::string header = "HTTP/1.1 200 OK\r\n" + connStr + "Content-Type: text/html\r\nContent-Length: " + std::to_string(fileStat.st_size) + "\r\n\r\n";
         outputBuffer_.append(header);
         fileLen_ = fileStat.st_size;
         fileOffset_ = 0;
      }
   }
}