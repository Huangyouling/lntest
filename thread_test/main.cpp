#include <iostream>
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>

std::mutex mtx;

void colorConversion(cv::Mat& img){
    std::lock_guard<std::mutex> lock(mtx); // 自动锁定互斥锁，确保线程安全
    std::cout << "-----子线程------" << std::endl;

    cv::cvtColor(img, img, cv::COLOR_RGB2RGBA);
}


int main(void){

    cv::Mat visibleImg = cv::Mat::zeros(cv::Size(1920,1080),CV_8UC3);
    cv::Mat infrareImg = cv::Mat::zeros(cv::Size(1920,1080),CV_8UC3);

    while (true){
    
        std::thread thread1(colorConversion, std::ref(visibleImg));
        std::thread thread2(colorConversion, std::ref(infrareImg));
        thread1.join();
        thread2.join();
        std::cout << "-----主线程------" << std::endl;
    }

    return 0;
}