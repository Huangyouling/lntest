#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    // 创建视频捕获对象
    cv::VideoCapture cap("/dev/video2"); // 2 表示打开第三个相机（根据你的系统和相机配置进行调整）
    // 检查相机是否成功打开
    if (!cap.isOpened()) {
        std::cout << "Failed to open camera." << std::endl;
        return -1;
    }
    int frameCount = 0; // 帧计数器
    while (true) {
        cv::Mat frame;
        // 从相机中读取帧
        cap.read(frame);
        // 检查帧是否为空
        if (frame.empty()) {
            std::cout << "Failed to capture frame." << std::endl;
            break;
        }
        // 保存帧为图像文件
        std::string fileName = "frame_" + std::to_string(frameCount) + ".jpg";
        cv::imwrite(fileName, frame);
        std::cout << "保存成功" << std::endl;
        // 显示帧
        cv::imshow("Camera", frame);
        // 按下 ESC 键退出循环
        if (cv::waitKey(1) == 27) {
            break;
        }
        frameCount++;
    }
    // 释放资源
    cap.release();
    return 0;
}


void DualSpecRtspServer::initRtsp(int argc, char *argv[], shared_ptr<DualSpectrumCameraDriverInterface> dualSpecCameraPtr){
    std::thread thread1(&DualSpecRtspServer::imageBuffer, this, dualSpecCameraPtr);
    thread1.detach();

    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    GMainLoop *loop;
    loop = g_main_loop_new(NULL, FALSE);

    gst_init (&argc, &argv);

    /* create a server instance */
    server = gst_rtsp_server_new();
    string port = m_rtspConfigMap["Visible"].port;
    gst_rtsp_server_set_service(server, port.c_str());

    mounts = gst_rtsp_server_get_mount_points(server);

    for(int i = 0; i < m_rtspConfigs.size(); ++i){
        std::string s = "/" + m_rtspConfigs[i].pushStream;
        factory = gst_rtsp_media_factory_new ();
        gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! textoverlay name=timeoverlay halignment=left valignment=top ! textoverlay name=infooverlay halignment=right valignment=bottom ! videoconvert ! v4l2h264enc ! rtph264pay name=pay0 pt=96 )");
        if(m_rtspConfigs[i].pushStream == "Visible"){
            g_signal_connect(factory, "media-configure", (GCallback) media_configure, NULL);
        }
        else if(m_rtspConfigs[i].pushStream == "Infrared"){
            g_signal_connect(factory, "media-configure", (GCallback) media_configureIr, NULL);
        }
        else{
            g_signal_connect(factory, "media-configure", (GCallback) media_configureFusion, NULL);
        }
        gst_rtsp_mount_points_add_factory(mounts, s.c_str(), factory);
    }

    /* don't need the ref to the mounts anymore */
    g_object_unref(mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

int DualSpecRtspServer::imageBuffer(shared_ptr<DualSpectrumCameraDriverInterface> dualSpecCameraPtr){
    std::string errStr;
    while(true){
        DualSpectrumImage dualSpectrumImage = dualSpecCameraPtr->getDualSpacImg(errStr);
        std::lock_guard<std::mutex> lock(*g_mutex);
        if(m_withfusion){
            cv::Mat fusionImg = m_fusion->fusion(dualSpectrumImage);
            m_img384_fusion = fusionImg;
        }

        cv::cvtColor(dualSpectrumImage.visibleImg.clone(), dualSpectrumImage.visibleImg, cv::COLOR_RGB2BGRA);
        cv::cvtColor(dualSpectrumImage.infraredImg.clone(), dualSpectrumImage.infraredImg, cv::COLOR_RGB2RGBA);
        m_img384_ir = dualSpectrumImage.infraredImg;
        m_img384_rgb = dualSpectrumImage.visibleImg;
    }
}

void DualSpecRtspServer::need_data(GstElement * appsrc, guint unused, MyContext *ctx){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    std::string deviceInfo = m_rtspConfigMap["Visible"].deviceInfo;
    size = m_img384_rgb.rows * m_img384_rgb.cols * 4;
    // 创建具有给定大小的预分配数据的缓冲区。
    buffer = gst_buffer_new_allocate(NULL, size, NULL);

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(*g_mutex);

    memcpy(map.data, m_img384_rgb.data, size);
    ctx->white = !ctx->white;

    /* increment the timestamp every 1/2 second */
    GST_BUFFER_PTS(buffer) = ctx->timestamp;

    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);

    /* 获取当前日期和时间的字符串 */
    std::string dateTimeStr = getCurrentDateTime(RtspServerConfigSet::convDateFormat(m_rtspConfigMap["Visible"].dateInfoFormat), m_rtspConfigMap["Visible"].timeFormat);

    /* 设置textoverlay的文本属性 */
    if(m_rtspConfigMap["Visible"].displayDate == "true"){
        g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
    }
    if(m_rtspConfigMap["Visible"].displayDevice == "true"){
        g_object_set(G_OBJECT (ctx->infooverlay), "text", deviceInfo.c_str(), NULL);
    }

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}

void DualSpecRtspServer::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
    GstElement *element, *appsrc, *timeoverlay, *infooverlay;
    MyContext *ctx;
    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element(media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc");

    timeoverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "timeoverlay");
    infooverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "infooverlay");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT (appsrc), "format", "time");

    /* configure the caps of the video */
    g_object_set(G_OBJECT (appsrc), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "BGRx",
                                     "width", G_TYPE_INT, m_img384_rgb.cols,
                                     "height", G_TYPE_INT, m_img384_rgb.rows,
                                     "framerate", GST_TYPE_FRACTION, 30, 1, NULL), NULL);

    g_object_set(G_OBJECT (timeoverlay), "halignment", "left", "valignment", "top", "xpad", 50, "ypad", 10, NULL);
    g_object_set(G_OBJECT (infooverlay), "halignment", "right", "valignment", "bottom", "xpad", 50, "ypad", 20, NULL);

    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    ctx->timeoverlay = timeoverlay;
    ctx->infooverlay = infooverlay;
    /* make sure ther datais freed when the media is gone */
    g_object_set_data_full(G_OBJECT (media), "my-extra-data", ctx, (GDestroyNotify) g_free);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", (GCallback) need_data, ctx);
    gst_object_unref(infooverlay);
    gst_object_unref(timeoverlay);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}
