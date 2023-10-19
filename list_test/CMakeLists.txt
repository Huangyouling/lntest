cmake_minimum_required(VERSION 3.10)
project(listtest)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(listtest main.cpp)
