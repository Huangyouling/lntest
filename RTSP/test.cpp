
class DualImage{
public:
    DualImage(){}
    virtual ~DualImage(){}

    shared_ptr<DualImage> getInstance(){
        if(m_DualImage == nullptr)
        {
            if(m_DualImage == nullptr){
                m_DualImage.reset(new DualImage);
            }
        }
        return m_DualImage;
    }
    void initDualImage(){
        std::string errStr;
        DualSpectrumManager::getInstance()->init();
        m_dualSpacCameraPtr = DualSpectrumManager::getInstance()->getDualSpectrumCamera();
        getImage();
    }

    void getImage(){
        string errStr;
        m_dualSpacCameraPtr->open();
        std::thread getImageThread(&DualImage::imageThread, this, std::ref(errStr));
        getImageThread.detach();
    }

    void imageThread(string errStr) {
        while (true) {
            DualSpectrumImage dualSepcImg = m_dualSpacCameraPtr->getDualSpacImg(errStr);
            m_DualImage->setDualSepcImg(dualSepcImg);
    }
}

    DualSpectrumImage getDualSepcImg() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_dualSepcImg;
    }

    void setDualSepcImg(const DualSpectrumImage& img) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_dualSepcImg = img;
    }

public:
    shared_ptr<DualSpectrumCameraDriverInterface> m_dualSpacCameraPtr;
    shared_ptr<DualImage> m_DualImage;

    shared_ptr<mutex> m_mutexPtr;
    DualSpectrumImage m_dualSepcImg;

    std::mutex m_mutex; 
};

typedef struct{
    gboolean white;
    GstClockTime timestamp;
    GstElement *timeoverlay;
    GstElement *infooverlay;
} MyContext;

class Rtsp{
public:
    Rtsp(){}
    virtual ~Rtsp(){}

    void init(int argc, char *argv[], shared_ptr<DualImage> dualImage){
        GMainLoop *loop;
        GstRTSPServer *server;
        GstRTSPMountPoints *mounts;
        GstRTSPMediaFactory *factory;

        Json::Reader reader;
        Json::Value root;
        //从文件中读取，保证当前文件有demo.json文件
        ifstream in(Constants::ConfigFileName, ios::binary);
        if(!in.is_open()){
            string errStr = DC_DualSpectrumErrorCoder::ERR_CODE_0005_Config_Open;
            Log_Error(errStr);
        }
        if(reader.parse(in, root)){
            m_dualRtspType = root["DualSpectRtspServer"]["RtspType"].asString();
            string matchmethod = root["DualSpectrumCamera"]["MatchMethod"].asString();
            if(root["DualSpectRtspServer"]["WithPip"].asString().empty() == false){
                m_withPip = root["DualSpectRtspServer"]["WithPip"].asString();
                m_pipMode = root["DualSpectRtspServer"]["PipMode"].asString();
                m_pip = CommonTools::getBoolFromString(m_withPip);
            }
            if(matchmethod == "CutCpuCaliber"){
                m_rtspHeight = root["VisibleCamera"]["VisibleCropY2"].asInt() - root["VisibleCamera"]["VisibleCropY1"].asInt();
                m_rtspWidth = root["VisibleCamera"]["VisibleCropX2"].asInt() - root["VisibleCamera"]["VisibleCropX1"].asInt();
            }else{
                m_rtspHeight = root["DualSpectrumCamera"]["VisibleCropY2"].asInt() - root["DualSpectrumCamera"]["VisibleCropY1"].asInt();
                m_rtspWidth = root["DualSpectrumCamera"]["VisibleCropX2"].asInt() - root["DualSpectrumCamera"]["VisibleCropX1"].asInt();
            }
            m_dualDevice = root["DualSpectRtspServer"]["dualDevice"].asString();
        }
        in.close();
        if(m_dualRtspType == "Single"){
            m_rtspWidth += m_rtspWidth;
        }
        m_deviceStr = getDualDevice();
        m_dualImage = dualImage;
        m_dualImage->initDualImage();
        gst_init (&argc, &argv);
        loop = g_main_loop_new(NULL, FALSE);
        server = gst_rtsp_server_new();
        mounts = gst_rtsp_server_get_mount_points(server);
        factory = gst_rtsp_media_factory_new ();
        gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! textoverlay name=timeoverlay halignment=left valignment=top ! textoverlay name=infooverlay halignment=right valignment=bottom ! nvvidconv ! nvv4l2h264enc profile=4 maxperf-enable=1 ! rtph264pay name=pay0 pt=96 )");
        g_signal_connect(factory, "media-configure", (GCallback) media_configure, NULL);
        gst_rtsp_mount_points_add_factory(mounts, "/rgb", factory);            //rgb
        g_object_unref(mounts);
        gst_rtsp_server_attach(server, NULL);
        g_print("stream ready at rtsp://127.0.0.1:8554/rgb\n");
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
    }

    static void getImage(){
        DualSpectrumImage dualSpectrumImage = m_dualImage->getDualSepcImg();
        m_image = dualSpectrumImage.visibleImg;
        cv::cvtColor(m_image.clone(), m_image, cv::COLOR_RGB2BGRA);
    }

    static void need_data(GstElement * appsrc, guint unused, MyContext * ctx){
        getImage();
        GstBuffer *buffer;
        guint size;
        GstFlowReturn ret;
        size = m_rtspWidth * m_rtspHeight * 4;
        buffer = gst_buffer_new_allocate(NULL, size, NULL);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);
        std::lock_guard<std::mutex> lock(*g_mutex);
        memcpy(map.data, m_image.data, size);
        ctx->white = !ctx->white;
        GST_BUFFER_PTS(buffer) = ctx->timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);
        ctx->timestamp += GST_BUFFER_DURATION(buffer);
        std::string dateTimeStr = getCurrentDateTime();
        g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
        g_object_set(G_OBJECT (ctx->infooverlay), "text", m_deviceStr.c_str(), NULL);
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        gst_buffer_unmap(buffer, &map);
        gst_buffer_unref(buffer);
    }

    static void media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
        GstElement *element, *appsrc, *timeoverlay, *infooverlay;
        MyContext *ctx;
        element = gst_rtsp_media_get_element(media);
        appsrc = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc");
        timeoverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "timeoverlay");
        infooverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "infooverlay");
        gst_util_set_object_arg(G_OBJECT (appsrc), "format", "time");
        g_object_set(G_OBJECT (appsrc), "caps",
                     gst_caps_new_simple("video/x-raw",
                                         "format", G_TYPE_STRING, "BGRx",
                                         "width", G_TYPE_INT, m_rtspWidth,
                                         "height", G_TYPE_INT, m_rtspHeight,
                                         "framerate", GST_TYPE_FRACTION, 30, 1, NULL), NULL);

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

    static string getCurrentDateTime(){
        auto t = std::time(nullptr); // 获取当前时间
        auto tm = *std::localtime(&t); // 转换为本地时间
        std::stringstream ss;
        ss << std::put_time(&tm, "%m/%d/%Y %A %H:%M:%S"); // 使用 %A 来获取星期的全名
        return ss.str();
    }

    string getDualDevice(){
        std::stringstream ss;
        ss << m_dualDevice;
        return ss.str();
    }

public:
    static string m_dualDevice;
    static string m_pip;
    static string m_deviceStr;
    static string m_withPip;
    static string m_dualRtspType;
    static string m_pipMode;
    static int m_rtspWidth;
    static int m_rtspHeight;
    static cv::Mat m_image;
    static DualSpectrumImage dualImage;
    static shared_ptr<DualImage> m_dualImage;
    static shared_ptr<mutex> g_mutex;

};


class DualSpecRPC_InterfaceHandler : virtual public DualSpecRPC_InterfaceIf {
public:
    DualSpecRPC_InterfaceHandler(shared_ptr<DualImage> dualImage){
        m_dualImage = dualImage;
    }

    void getImage(){
        DualSpectrumImage dualSepcImgtemp = m_dualImage->getDualSepcImg();
        m_visibleImg = dualSepcImgtemp.visibleImg.clone();
        m_infraredImg = dualSepcImgtemp.infraredImg.clone();
        cv::cvtColor(m_visibleImg, m_visibleImg, cv::COLOR_BGR2RGB);
    }

    void getFrame(ImgData& _return) {
        getImage();
        std::string dateTimeStr = Rtsp::getCurrentDateTime();

        cv::putText(m_visibleImg, dateTimeStr, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        std::string imgStrRgb((char* )m_visibleImg.data, m_visibleImg.total() * m_visibleImg.elemSize());
        std::string localImgStrRgb = imgStrRgb;
        _return.rgbImg = localImgStrRgb;
        _return.rgbWidth = m_visibleImg.cols;
        _return.rgbHeight = m_visibleImg.rows;

    }

    bool open() {
        m_dualImage->initDualImage();
        return  m_dualImage->m_dualSpacCameraPtr->open().empty();
        Log_Info("open ");
    }

    bool close() {
        return m_dualImage->m_dualSpacCameraPtr->close().empty();
        Log_Info("close ");
    }

public:
    shared_ptr<DualImage> m_dualImage;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    cv::Mat m_visibleImg;
    cv::Mat m_infraredImg;
    shared_ptr<mutex> g_mutex;
};

string Rtsp::m_dualDevice = "";
string Rtsp::m_pip = "";
string Rtsp::m_deviceStr = "";
string Rtsp::m_withPip = "";
string Rtsp::m_dualRtspType = "";
string Rtsp::m_pipMode = "";
int Rtsp::m_rtspWidth = 0;
int Rtsp::m_rtspHeight = 0;
cv::Mat Rtsp::m_image;
DualSpectrumImage Rtsp::dualImage;
shared_ptr<mutex> Rtsp::g_mutex = make_shared<mutex>();
shared_ptr<DualImage> Rtsp::m_dualImage;

int main(int argc, char *argv[]){
    DualImage dualImage;
    std::shared_ptr<DualImage> dualImageptr = dualImage.getInstance();
    std::thread rtspThread([&](){
        Rtsp rtsp;
        rtsp.init(argc, argv, dualImageptr);
    });
    int port = 9091;
    std::thread rpcThread([&]() {
        ::std::shared_ptr<DualSpecRPC_InterfaceHandler> handler(new DualSpecRPC_InterfaceHandler(dualImageptr));
        ::std::shared_ptr<TProcessor> processor(new DualSpecRPC_InterfaceProcessor(handler));
        ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
        ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
        ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
        TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
        server.serve();
        Log_Info("server port :{}", port);
        Log_Info("DualSpecRPC service start succeeded !");
    });
    rtspThread.join();
    rpcThread.join();
    return 0;
}


#include <stdio.h>

int main(){
    printf("hello world!");
    return 0;
}


#include <iostream>

int main(){
    std::cout << "hello world!" << std::endl;
    return 0;
}