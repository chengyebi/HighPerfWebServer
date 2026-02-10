#include "HttpRequest.h"
#include "Buffer.h"
#include <iostream>
void HttpRequest::init() {
    method_="";
    path_="";
    version_="";
    body_="";
    state_=REQUEST_LINE;
    headers_.clear();
}
bool HttpRequest::isKeepAlive() const {
    if (headers_.count("Connection")) {
        return headers_.find("Connection")->second=="keep-alive"&&version_=="1.1";
    }
    return false;
}
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[]="\r\n";
    if (buff.readableBytes()<=0) return false;
    //只要buffer里有数据，且状态没有结束，就一直解析
    while (buff.readableBytes()&&state_!=FINISH) {
        //1.获取一行数据，从buffer读指针开始，找第一个\r\n
        const char* lineEnd=std::search(buff.peek(),buff.peek()+buff.readableBytes(),CRLF,CRLF+2);
        //没找到\r\n，说明这一行数据还没收全，TCP拆包了，直接返回等下次
        if (lineEnd==buff.peek()+buff.readableBytes()) {
            break;
        }
        //取出这一行字符串
        std::string line(buff.peek(),lineEnd);
        //根据状态机处理这一行：
        switch (state_) {
            case REQUEST_LINE:
                if (!parseRequestLine_(line)) {
                    return false;//请求格式错误，解析失败
                }
                parsePath_();//格式不错误，就开始解析路径
                break;
            case HEADERS:
                parseHeader_(line);
                if (line.empty()) {
                    //这一行是空行说明Header结束了
                    state_=FINISH;
                }
                break;
            case BODY:
                parseBody_(line);
                break;
            default:
                break;
        }
        //读完一行，Buffer指针往后跳过这一行+\r\n(2字节)
        buff.retrieveUntil(lineEnd+2);
    }
    return true;
}
//手动解析请求行：GET/index.html HTTP/1.1
bool HttpRequest::parseRequestLine_(const std::string& line) {
    //找第一个空格
    size_t methodEnd=line.find(' ');
    if (methodEnd==std::string::npos) return false;
    method_=line.substr(0,methodEnd);
    //找第二个空格
    size_t pathEnd=line.find(' ',methodEnd+1);
    if (pathEnd==std::string::npos) return false;
    path_=line.substr(methodEnd+1,pathEnd-methodEnd-1);
    //剩下的就是版本号
    version_=line.substr(pathEnd+1);
    //状态流转：
    //进行解析下一部分：header
    state_=HEADERS;
    return true;
}
//手动解析Header:Host:127.0.0.1
void HttpRequest::parseHeader_(const std::string& line) {
    size_t colon=line.find(':');
    if (colon==std::string::npos) {
        return;
    }
    //冒号前是key
    std::string key=line.substr(0,colon);
    //冒号后是value,通常会有个空格，要去掉
    size_t valStart=colon+1;
    while (valStart<line.size()&&line[valStart]==' ') {
        valStart++;
    }
    std::string value=line.substr(valStart);
    headers_[key]=value;
}
void HttpRequest::parseBody_(const std::string& line) {
    body_=line;
    state_=FINISH;
}
//简单的路径处理
void HttpRequest::parsePath_() {
    if (path_=="/") {
        path_="/index.html";
    }
}
std::string HttpRequest::path() const {
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}
std::string HttpRequest::version() const {
    return version_;
}
std::string HttpRequest::getHeader(const std::string& key) const {
    if (headers_.count(key)) return headers_.at(key);
    return "";
}
