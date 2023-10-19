cmake_minimum_required(VERSION 3.10)
project(threadtest)



# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED)


# 添加可执行文件
add_executable(threadtest main.cpp)
target_link_libraries(threadtest ${OpenCV_LIBS})
target_link_libraries(threadtest pthread)
