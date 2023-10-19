#include "RTSPCommont.h"


RTSPCommont::RTSPCommont(){

}

RTSPCommont::~RTSPCommont(){

}

void RTSPCommont::init(string pushMode){
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    gst_init (&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    server = gst_rtsp_server_new();
    mounts = gst_rtsp_server_get_mount_points(server);

    /* make a media factory for a test stream. The default media factory can use
    * gst-launch syntax to create pipelines.
    * any launch line works as long as it contains elements named pay%d. Each
    * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new ();

    /* notify when our media is ready, This is called whenever someone asks for
    * the media and a new pipeline with our appsrc is created */
    gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! textoverlay name=timeoverlay halignment=left valignment=top ! textoverlay name=infooverlay halignment=right valignment=bottom ! nvvidconv ! nvv4l2h264enc profile=4 maxperf-enable=1 ! rtph264pay name=pay0 pt=96 )");
    g_signal_connect(factory, "media-configure", (GCallback) media_configure, NULL);

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory(mounts, pushMode, factory);

    /* don't need the ref to the mounts anymore */
    g_object_unref(mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);

    g_print("stream ready at rtsp://127.0.0.1:8554/" + pushMode + "\n");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

void RTSPCommont::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
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
    /* make sure ther datais freed when the media is gone */
    g_object_set_data_full(G_OBJECT (media), "my-extra-data", ctx, (GDestroyNotify) g_free);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", (GCallback) need_data, ctx);
    gst_object_unref(infooverlay);
    gst_object_unref(timeoverlay);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}

void RTSPCommont::need_data(GstElement * appsrc, guint unused, MyContext * ctx){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;

    size = DualSpecRtspServer::rtspWidth * DualSpecRtspServer::rtspHeight * 4;
    // 创建具有给定大小的预分配数据的缓冲区。
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    std::lock_guard<std::mutex> lock(*g_mutex);

    memcpy(map.data, image.data, size);

    ctx->white = !ctx->white;

    /* increment the timestamp every 1/2 second */
    GST_BUFFER_PTS(buffer) = ctx->timestamp;

    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);

    /* 获取当前日期和时间的字符串 */
    std::string dateTimeStr = getCurrentDateTime();
    std::string deviceStr = getDualDevice();

    /* 设置textoverlay的文本属性 */
    g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
    g_object_set(G_OBJECT (ctx->infooverlay), "text", deviceStr.c_str(), NULL);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}

string RTSPCommont::getCurrentDateTime(){
    auto t = std::time(nullptr); // 获取当前时间
    auto tm = *std::localtime(&t); // 转换为本地时间
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %A %H:%M:%S"); // 使用 %A 来获取星期的全名
    return ss.str();
}

string RTSPCommont::getDevice(){
    std::stringstream ss;
    ss << "device";
    return ss.str();
}
