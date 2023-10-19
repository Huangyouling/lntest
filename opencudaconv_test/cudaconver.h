
#ifndef CUDACONVER_H
#define CUDACONVER_H

#include <cuda_runtime.h>

__global__ void rgb2rgbaKernel(const uchar3* src, uchar4* dst, int width, int height);

void kernle_convertRGBtoRGBA(const cv::Mat& rgbImage, cv::Mat& rgbaImage);

#endif // CUDACONVER_H
