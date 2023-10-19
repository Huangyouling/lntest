#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

int main()
{
    #if 0
    // 创建每天一个文件的日志输出器
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("../logs/logfile", 0, 0, false, 10);

    // 创建输出到控制台的带颜色日志输出器
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // 创建日志记录器并添加日志输出器
    auto logger = std::make_shared<spdlog::logger>("daily_logger", spdlog::sinks_init_list{file_sink, console_sink});

    // 设置日志级别为 info 及以上
    logger->set_level(spdlog::level::info);

    // 记录不同级别的日志消息
    logger->trace("This is a trace message.");
    logger->debug("This is a debug message.");
    logger->info("This is an info message.");
    logger->warn("This is a warning message.");
    logger->error("This is an error message.");

    // 注册日志器
    spdlog::register_logger(logger);

    // 设置默认日志器
    spdlog::set_default_logger(logger);

    // 刷新和关闭日志器
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::shutdown();
    #endif

    // 创建按日期轮转的文件日志输出器
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../logs/logfile.txt", 1024 * 100, 10, true);

    // 创建日志记录器并添加日志输出器
    auto logger = std::make_shared<spdlog::logger>("logger", file_sink);

    // 设置日志级别为 info 及以上
    logger->set_level(spdlog::level::info);

    // 记录不同级别的日志消息
    logger->info("This is an info message.");
    logger->warn("This is a warning message.");
    logger->error("This is an error message.");

    // 注册日志器
    spdlog::register_logger(logger);

    // 设置默认日志器
    spdlog::set_default_logger(logger);

    // 刷新和关闭日志器
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::shutdown();


#include "DC_DualSpecRTSPServer.h"
#include "DualSpectrumImageFusioner.h"
#include "constants.h"
#include "json/json.h"
#include "configsetting.h"
#include "DC_DualSpectrumErrorCode.h"
#include <fstream>
#include "DualSpectrumImageFusioner.h"
#include "DCTime.h"
#include "DCLog.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cuda.h>
#include <opencv2/core/cuda.hpp>
#include <opencv2/opencv.hpp>
#include <opencv4/opencv2/cudaimgproc.hpp>
#include <ctime>

cv::Mat  DualSpecRtspServer::img384_buffer[2];
std::atomic<int> DualSpecRtspServer::index;
shared_ptr<DualSpecRtspServer>DualSpecRtspServer::m_DualSpecRtspServer = shared_ptr<DualSpecRtspServer>();
shared_ptr<mutex> DualSpecRtspServer::m_mutexPtr = shared_ptr<mutex>(new mutex);
shared_ptr<mutex> DualSpecRtspServer::g_mutex = shared_ptr<mutex>(new mutex);
cv::Mat  DualSpecRtspServer::img384_ir;
cv::Mat  DualSpecRtspServer::img384_rgb;
cv::Mat  DualSpecRtspServer::img384;
cv::Mat  DualSpecRtspServer::img384_fusion;
string DualSpecRtspServer::dualRtspType = "";
string DualSpecRtspServer::dualDevice = "";
int DualSpecRtspServer::rtspHeight = 1069;
int DualSpecRtspServer::rtspWidth = 1499;
string DualSpecRtspServer::getCurrentDateTime(){
    auto t = std::time(nullptr); // 获取当前时间
    auto tm = *std::localtime(&t); // 转换为本地时间
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %A %H:%M:%S"); // 使用 %A 来获取星期的全名
    return ss.str();
}
string DualSpecRtspServer::getDualDevice(){
    std::stringstream ss;
    ss << DualSpecRtspServer::dualDevice;
    return ss.str();
}
DualSpecRtspServer::DualSpecRtspServer(){}
void DualSpecRtspServer::colorConversion(cv::Mat& img){
    cv::cvtColor(img, img, cv::COLOR_RGB2RGBA);
}
shared_ptr<DualSpecRtspServer> DualSpecRtspServer::getInstance(){
    if (m_DualSpecRtspServer == nullptr)
    {
        lock_guard<mutex> lock(*m_mutexPtr);
        if (m_DualSpecRtspServer == nullptr)
        {
            m_DualSpecRtspServer.reset(new DualSpecRtspServer);
        }
    }
    return m_DualSpecRtspServer;
}
void DualSpecRtspServer::initRtsp(int argc, char *argv[])
{
    std::thread thread1(&DualSpecRtspServer::readthreadFun, DualSpecRtspServer::getInstance());
    thread1.detach();
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    Json::Reader reader;
    Json::Value root;
    ifstream in(Constants::ConfigFileName, ios::binary);
    if(!in.is_open()){
        string errStr = DC_DualSpectrumErrorCoder::ERR_CODE_0005_Config_Open;
        Log_Error(errStr);
    }
    if(reader.parse(in, root)){
        DualSpecRtspServer::getInstance()->dualRtspType = root["DualSpectRtspServer"]["RtspType"].asString();
        DualSpecRtspServer::rtspHeight = root["DualSpectrumCamera"]["VisibleCropY2"].asInt() - root["DualSpectrumCamera"]["VisibleCropY1"].asInt();
        DualSpecRtspServer::rtspWidth = root["DualSpectrumCamera"]["VisibleCropX2"].asInt() - root["DualSpectrumCamera"]["VisibleCropX1"].asInt();
        DualSpecRtspServer::getInstance()->dualDevice = root["DualSpectRtspServer"]["dualDevice"].asString();
    }
    if(dualRtspType == "Single"){
        DualSpecRtspServer::rtspWidth += DualSpecRtspServer::rtspWidth;
    }
    gst_init (&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);
    server = gst_rtsp_server_new();
    mounts = gst_rtsp_server_get_mount_points(server);
    factory = gst_rtsp_media_factory_new ();
    gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! textoverlay name=timeoverlay halignment=left valignment=top color=white ! videoconvert ! textoverlay name=infooverlay halignment=right valignment=bottom color=white ! nvvidconv ! nvv4l2h264enc profile=4 maxperf-enable=1 ! rtph264pay name=pay0 pt=96 )");
    g_signal_connect(factory, "media-configure", (GCallback) media_configure, NULL);
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);
    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}
int DualSpecRtspServer::readthreadFun(){
    std::string errStr;
    DualSpectrumManager::getInstance()->init();
    shared_ptr<DualSpectrumCameraDriverInterface> dualSpecCameraPtr =
            DualSpectrumManager::getInstance()->getDualSpectrumCamera();
    if(dualSpecCameraPtr == nullptr){
        return 0;
    }
    DualSpectrumImageFusioner fusion = DualSpectrumImageFusioner();
    fusion.setFusionMode(FusionMode_EQUAL_QUADRUPLE);
    dualSpecCameraPtr->open();
    std::locale::global(std::locale("zh_CN.utf8")); // 设置全局的 locale 为中文
    while(true){
        DualSpectrumImage dualSpecImage = dualSpecCameraPtr->getDualSpacImg(errStr);
        cv::Mat visibleImg = dualSpecImage.visibleImg;
        cv::Mat infrareImg = dualSpecImage.infraredImg;
        cv::Mat fusionImag = fusion.fusion(dualSpecImage);
        if(!infrareImg.empty() && !visibleImg.empty()){
            colorConversion(visibleImg);
            colorConversion(infrareImg);
            std::lock_guard<std::mutex> lock(*g_mutex);
            if (dualRtspType == "Single"){
                cv::Mat outputImage(DualSpecRtspServer::rtspHeight, DualSpecRtspServer::rtspWidth, visibleImg.type());
                cv::hconcat(visibleImg, infrareImg, outputImage);
                img384 = outputImage;
            }
            else if (dualRtspType == "Multichannel"){
                img384_ir = infrareImg;
                img384_rgb = visibleImg;
                img384_fusion = fusionImag;
            }
            else{Log_Error("RTSP MODE ERROR");}
        }
        else{
            Log_Error("infrareImg and visibleImg empty");
        }
    }
}
void DualSpecRtspServer::need_data(GstElement * appsrc, guint unused, MyContext * ctx){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    size = DualSpecRtspServer::rtspWidth * DualSpecRtspServer::rtspHeight * 4;
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(*g_mutex);
    if(dualRtspType == "Single"){
        memcpy(map.data, img384.data, size);
    }
    else{
        memcpy(map.data, img384_rgb.data, size);
    }
    ctx->white = !ctx->white;
    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);
    std::string dateTimeStr = DualSpecRtspServer::getInstance()->getCurrentDateTime();
    std::string deviceStr = DualSpecRtspServer::getInstance()->getDualDevice();
    g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
    g_object_set(G_OBJECT (ctx->infooverlay), "text", deviceStr.c_str(), NULL);
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}
void DualSpecRtspServer::need_dataIr(GstElement * appsrc, guint unused, MyContext * ctx){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    size = DualSpecRtspServer::rtspWidth * DualSpecRtspServer::rtspHeight * 4;
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(*g_mutex);
    memcpy(map.data, img384_ir.data, size);
    ctx->white = !ctx->white;
    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);
    std::string dateTimeStr = DualSpecRtspServer::getInstance()->getCurrentDateTime();
    std::string deviceStr = DualSpecRtspServer::getInstance()->getDualDevice();
    g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
    g_object_set(G_OBJECT (ctx->infooverlay), "text", deviceStr.c_str(), NULL);
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}
void DualSpecRtspServer::need_dataFusion(GstElement * appsrc, guint unused, MyContext * ctx){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    size = DualSpecRtspServer::rtspWidth * DualSpecRtspServer::rtspHeight * 4;
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(*g_mutex);
    memcpy(map.data,img384_fusion.data, size);
    ctx->white = !ctx->white;
    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);
    std::string dateTimeStr = DualSpecRtspServer::getInstance()->getCurrentDateTime();
    std::string deviceStr = DualSpecRtspServer::getInstance()->getDualDevice();
    g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
    g_object_set(G_OBJECT (ctx->infooverlay), "text", deviceStr.c_str(), NULL);
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}
void DualSpecRtspServer::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
    GstElement *element, *appsrc, *timeoverlay, *infooverlay;
    MyContext *ctx;
    element = gst_rtsp_media_get_element(media);
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc");
    timeoverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "timeoverlay");
    infooverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "infooverlay");
    gst_util_set_object_arg(G_OBJECT (appsrc), "format", "time");
    g_object_set(G_OBJECT (appsrc), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGBA",
                                     "width", G_TYPE_INT, DualSpecRtspServer::rtspWidth,
                                     "height", G_TYPE_INT, DualSpecRtspServer::rtspHeight,
                                     "framerate", GST_TYPE_FRACTION, fps, 1, NULL), NULL);
    g_object_set(G_OBJECT (timeoverlay), "halignment", "left", "valignment", "top", "xpad", 50, "ypad", 10, NULL);
    g_object_set(G_OBJECT (infooverlay), "halignment", "right", "valignment", "bottom", "xpad", 50, "ypad", 20, NULL);
    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    ctx->timeoverlay = timeoverlay;
    ctx->infooverlay = infooverlay;
    g_object_set_data_full(G_OBJECT (media), "my-extra-data", ctx, (GDestroyNotify) g_free);
    g_signal_connect(appsrc, "need-data", (GCallback) need_data, ctx);
    gst_object_unref(infooverlay);
    gst_object_unref(timeoverlay);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}
void DualSpecRtspServer::media_configureIr(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
    GstElement *element, *appsrc, *timeoverlay, *infooverlay;
    MyContext *ctx;
    element = gst_rtsp_media_get_element(media);
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc");
    timeoverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "timeoverlay");
    infooverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "infooverlay");
    gst_util_set_object_arg(G_OBJECT (appsrc), "format", "time");
    g_object_set(G_OBJECT (appsrc), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGBA",
                                     "width", G_TYPE_INT, DualSpecRtspServer::rtspWidth,
                                     "height", G_TYPE_INT, DualSpecRtspServer::rtspHeight,
                                     "framerate", GST_TYPE_FRACTION, fps, 1, NULL), NULL);
    g_object_set(G_OBJECT (timeoverlay), "halignment", "left", "valignment", "top", "xpad", 50, "ypad", 10, NULL);
    g_object_set(G_OBJECT (infooverlay), "halignment", "right", "valignment", "bottom", "xpad", 50, "ypad", 20, NULL);
    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    ctx->timeoverlay = timeoverlay;
    ctx->infooverlay = infooverlay;
    g_object_set_data_full(G_OBJECT (media), "my-extra-data", ctx, (GDestroyNotify) g_free);
    g_signal_connect(appsrc, "need-data", (GCallback) need_dataIr, ctx);
    gst_object_unref(infooverlay);
    gst_object_unref(timeoverlay);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}
void DualSpecRtspServer::media_configureFusion(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
    GstElement *element, *appsrc, *timeoverlay, *infooverlay;
    MyContext *ctx;
    element = gst_rtsp_media_get_element(media);
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc");
    timeoverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "timeoverlay");
    infooverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "infooverlay");
    gst_util_set_object_arg(G_OBJECT (appsrc), "format", "time");
    g_object_set(G_OBJECT (appsrc), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGBA",
                                     "width", G_TYPE_INT, DualSpecRtspServer::rtspWidth,
                                     "height", G_TYPE_INT, DualSpecRtspServer::rtspHeight,
                                     "framerate", GST_TYPE_FRACTION, fps, 1, NULL), NULL);
    g_object_set(G_OBJECT (timeoverlay), "halignment", "left", "valignment", "top", "xpad", 50, "ypad", 10, NULL);
    g_object_set(G_OBJECT (infooverlay), "halignment", "right", "valignment", "bottom", "xpad", 50, "ypad", 20, NULL);
    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    ctx->timeoverlay = timeoverlay;
    ctx->infooverlay = infooverlay;
    g_object_set_data_full(G_OBJECT (media), "my-extra-data", ctx, (GDestroyNotify) g_free);
    g_signal_connect(appsrc, "need-data", (GCallback) need_dataFusion, ctx);
    gst_object_unref(infooverlay);
    gst_object_unref(timeoverlay);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}















#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>

std::mutex bufferMutex;
cv::Mat buffer1;
cv::Mat buffer2;
bool pingPongFlag = false;
cv::Mat dstImg = cv:: Mat(640,512,CV_8UC3);
void resizeThread()
{
    while (true)
    {
       
        if(!buffer1.empty()|| !buffer2.empty())
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            if (pingPongFlag)
            {
                cv::resize(buffer1,dstImg,cv::Size(640,512));
            }
            else
            {
                cv::resize(buffer2,dstImg,cv::Size(640,512));
            }
        }
    }
}

void captureThread()
{
    cv::VideoCapture cap(0); // 用于读取视频帧的对象，这里假设摄像头设备为0

    while (true)
    {
        cv::Mat frame;
        cap.read(frame); // 读取视频帧
        if(!frame.empty())
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            if (pingPongFlag)
            {
                buffer2 = frame.clone();
                cap.read(buffer1); // 读取下一帧到缓冲区1
            }
            else
            {
                buffer1 = frame.clone();
                cap.read(buffer2); // 读取下一帧到缓冲区2
            }
            pingPongFlag = !pingPongFlag;
        }

    }
}

int main()
{
    std::cout << "开始读取相机视频并进行resize" << std::endl;
    
    std::thread captureThreadObj(captureThread);
    std::thread resizeThreadObj(resizeThread);
    // 主线程可以做其他的操作
    resizeThreadObj.join();
    captureThreadObj.join();

    std::cout << "程序结束" << std::endl;

    return 0;
}