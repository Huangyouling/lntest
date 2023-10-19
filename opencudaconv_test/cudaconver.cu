#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>
#include "cudaconver.h"

__global__ void rgb2rgbaKernel(const uchar3* src, uchar4* dst, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height) {
        int index = y * width + x;
        uchar3 rgb = src[index];
        dst[index] = make_uchar4(rgb.x, rgb.y, rgb.z, 255);
    }
}

void kernle_convertRGBtoRGBA(const cv::Mat& rgbImage, cv::Mat& rgbaImage){
    // 获取图像尺寸
    int width = rgbImage.cols;
    int height = rgbImage.rows;

    // 计算数据大小
    size_t size = width * height * sizeof(uchar3);

    // 分配CUDA内存
    uchar3* d_src;
    cudaMalloc((void**)&d_src, size);

    // 将RGB图像数据复制到CUDA内存
    cudaMemcpy(d_src, rgbImage.ptr(), size, cudaMemcpyHostToDevice);

    // 分配输出内存
    uchar4* d_dst;
    cudaMalloc((void**)&d_dst, size);

    // 定义线程块和网格的大小
    dim3 threadsPerBlock(16, 16);
    dim3 numBlocks((width + threadsPerBlock.x - 1) / threadsPerBlock.x, (height + threadsPerBlock.y - 1) / threadsPerBlock.y);

    // 调用CUDA核函数进行转换
    rgb2rgbaKernel<<<numBlocks, threadsPerBlock>>>(d_src, d_dst, width, height);

    // 将结果复制回主机内存
    cudaMemcpy(rgbaImage.ptr(), d_dst, size, cudaMemcpyDeviceToHost);

    // 释放CUDA内存
    cudaFree(d_src);
    cudaFree(d_dst);
}
