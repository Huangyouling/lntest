cmake_minimum_required(VERSION 3.5)
project(showimagedata)

# 设置编译选项
set(CMAKE_CXX_STANDARD 17)

# 寻找OpenCV包
find_package(OpenCV REQUIRED)


# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加可执行文件
add_executable(showimagedata main.cpp)

# 链接OpenCV库
target_link_libraries(showimagedata ${OpenCV_LIBS})

