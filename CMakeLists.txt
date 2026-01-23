cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

# 设置 C++ 标准为 C++17 (为了用更爽的语法)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件路径 (告诉编译器去 include 文件夹找 .h)
include_directories(include)

# 自动搜集 src 目录下所有的 .cpp 文件
file(GLOB SOURCES "src/*.cpp")

# 生成可执行文件：名字叫 server，由 main.cpp 和 src 下的所有文件编译而来
add_executable(server main.cpp ${SOURCES}
        src/test.h
)