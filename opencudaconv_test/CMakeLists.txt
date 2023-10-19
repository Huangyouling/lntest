cmake_minimum_required(VERSION 3.10)
project(rgb2rgba)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找 CUDA 包
find_package(CUDA REQUIRED)

# 添加可执行文件
add_executable(rgb2rgba main.cpp cudaconver.h)

# 设置 CUDA 相关属性
set_target_properties(rgb2rgba PROPERTIES CUDA_ARCHITECTURES "61")  # 根据您的 GPU 架构进行设置

# 添加 CUDA 源文件
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++11")
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -O3")
CUDA_ADD_LIBRARY(cudaconver cudaconver.cu)
target_link_libraries(rgb2rgba PRIVATE cudaconver)

# 链接 OpenCV 库
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(rgb2rgba PRIVATE ${OpenCV_LIBS})
