cmake_minimum_required(VERSION 2.8)
project(mylog)


# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 查找所要的库
# find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)

# 添加可执行文件
add_executable(mylog main.cpp)

# 连接需要的库
# target_link_libraries(mylog ${OpenCV_LIBS})
target_link_libraries(mylog ${spdlog_LIBS})
target_link_libraries(mylog pthread)

