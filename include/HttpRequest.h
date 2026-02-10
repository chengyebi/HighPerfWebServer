#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>
 class Buffer;
//前置声明
class HttpRequest {
public:
 bool isComplete() const {return state_==FINISH;}
 //HTTP请求的解析状态
 enum ParseState {
  REQUEST_LINE,//解析请求行
  HEADERS,//解析请求头
  BODY,//解析请求体
  FINISH,//完成
 };

 HttpRequest(){init();}
 ~HttpRequest()=default;

 void init();
 //核心接口，解析Buffer里的数据
 bool parse(Buffer& buff);

  //获取解析接口的数据
  std::string path() const;
  std::string method() const;
  std::string version() const;
  std::string getHeader(const std::string& key) const;
  //判断是不是长连接
  bool isKeepAlive() const;
private:
  //解析具体部分的私有方法
  bool parseRequestLine_(const std::string& line);
 void parseHeader_(const std::string& line);
 void parseBody_(const std::string& line);
 //处理路径，比如把/变成/index.html这种
 void parsePath_();
 ParseState state_;
 std::string method_;
 std::string path_;
 std::string version_;
 std::string body_;
 std::unordered_map<std::string,std::string> headers_;
};