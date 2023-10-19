#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaimgproc.hpp>
#include "cudaconver.h"

void gpuconvertRGBtoRGBA(const cv::Mat& inputImg, cv::Mat& outputImg){

    cv::cuda::Stream stream;
    CV_Assert(inputImg.channels() == 3 && inputImg.depth() == CV_8U);

    // 分配输出图像内存
    outputImg.create(inputImg.size(), CV_8UC4);

    // 使用CUDA加速
    cv::cuda::GpuMat gpuInput(inputImg);
    cv::cuda::GpuMat gpuOutput(outputImg);

    cv::cuda::cvtColor(gpuInput, gpuOutput, cv::COLOR_RGB2RGBA, 0, stream);

    gpuOutput.download(outputImg);

    stream.waitForCompletion();

}

void convertRGBtoRGBA(cv::Mat& outputImg){
    cv::cvtColor(outputImg, outputImg, cv::COLOR_RGB2RGBA);
}


int main(void){

    cv::Mat rgbImg = cv::imread("/home/hyl/datav/test/opencudaconv_test/dog.png", cv::IMREAD_COLOR);
    if (rgbImg.empty()){
        std::cout << "open image fail"  << std::endl;
        return -1;
    }
    cv::Mat rgbImg2 = rgbImg.clone();

    // 转换为RGBA
    cv::Mat rgbaImg;
    cv::Mat rgbaImg2;
    cv::Mat rgbaImg3(rgbImg.size(), CV_8UC4);


    cv::TickMeter tm;
    cv::TickMeter tm1;
    cv::TickMeter tm2;

    tm.start();
    gpuconvertRGBtoRGBA(rgbImg, rgbaImg);
    tm.stop();
    std::cout << "gpuConversion time: " << tm.getTimeSec() << " seconds" << std::endl;

    tm1.start();
    convertRGBtoRGBA(rgbImg2);
    tm1.stop();
    std::cout << "Conversion time: " << tm1.getTimeSec() << " seconds" << std::endl;

    tm2.start();
    kernle_convertRGBtoRGBA(rgbImg, rgbaImg3);
    tm2.stop();
    std::cout << "kernleConversion time: " << tm2.getTimeSec() << " seconds" << std::endl;

    cv::imwrite("output.png", rgbaImg);


    return 0;
}