cmake_minimum_required(VERSION 3.0)
project(CameraCapture)

# 查找 OpenCV 库
find_package(OpenCV REQUIRED)

# 添加可执行文件
add_executable(CameraCapture test.cpp)

# 链接 OpenCV 库
target_link_libraries(CameraCapture ${OpenCV_LIBS})
