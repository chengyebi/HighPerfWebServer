cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件路径,
include_directories(include)

# 生成可执行文件：名字叫 server，由 main.cpp 和 src 下的所有文件编译而来
add_executable(server
        main.cpp
        src/InetAddress.cpp
        src/Socket.cpp
        src/Epoll.cpp
        src/Buffer.cpp
        src/HttpRequest.cpp
        src/HttpConnection.cpp
)