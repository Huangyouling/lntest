typedef struct{
    gboolean white;
    GstClockTime timestamp;
    GstElement *timeoverlay;
    GstElement *infooverlay;
    GstElement *videobox;
    GstElement *videobox_pip;
    gint xpos;
    gint ypos;
} MyContext;
void DualSpecRtspServer::initRtsp(int argc, char *argv[]){
    std::thread thread1(&DualSpecRtspServer::readthreadFun, DualSpecRtspServer::getInstance());
    thread1.detach();
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    DualSpecRtspServer::pingPongFlagRtsp =false;
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
    gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc_main ! textoverlay name=timeoverlay_main halignment=left valignment=top ! \
                    textoverlay name=infooverlay_main halignment=right valignment=bottom ! nvvidconv ! nvv4l2h264enc profile=4 maxperf-enable=1 ! \
                    queue ! videomixer name=mix ! rtph264pay name=pay0 pt=96 " \
                    "appsrc name=mysrc_pip ! textoverlay name=timeoverlay_pip halignment=left valignment=top ! \
                    textoverlay name=infooverlay_pip halignment=right valignment=bottom ! nvvidconv ! nvv4l2h264enc profile=4 maxperf-enable=1 ! queue ! mix. )");

    
    g_signal_connect(factory, "media-configure", (GCallback) media_configure, NULL);
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);            //rgb+ir  /rgb
    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);
    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}
void DualSpecRtspServer::drawImage(GstElement * appsrc, guint unused, MyContext * ctx, cv::Mat image){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    size = DualSpecRtspServer::rtspWidth * DualSpecRtspServer::rtspHeight * 4;
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(*g_mutex);
    memcpy(map.data, image.data, size);
    ctx->white = !ctx->white;
    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);
    std::string dateTimeStr = DualSpecRtspServer::getInstance()->getCurrentDateTime();
    std::string deviceStr = DualSpecRtspServer::getInstance()->getDualDevice();
    g_object_set(G_OBJECT (ctx->timeoverlay), "text", dateTimeStr.c_str(), NULL);
    g_object_set(G_OBJECT (ctx->infooverlay), "text", deviceStr.c_str(), NULL);
    gst_child_proxy_set(G_OBJECT (ctx->videobox), "sink_0::xpos", 0, "sink_0::ypos", 0, NULL);
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}
void DualSpecRtspServer::need_data(GstElement * appsrc, guint unused, MyContext * ctx){
    if(dualRtspType == "Single"){
        drawImage(appsrc, unused, ctx, img384);
    }
    else{
        drawImage(appsrc, unused, ctx, img384_rgb);
    }
}
void DualSpecRtspServer::need_data_pip(GstElement * appsrc_pip, guint unused, MyContext * ctx){
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    size = 200 * 200 * 4;
    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(*g_mutex);
    image = cv.resize(ima, (200, 200));
    memcpy(map.data, image.data, size);
    ctx->white = !ctx->white;
    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);
    g_signal_emit_by_name(appsrc_pip, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}
void DualSpecRtspServer::setmedia_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data, int channel){
    GstElement *element, *appsrc_main, *appsrc_pip, *timeoverlay, *infooverlay, *videobox, *videobox_pip;
    MyContext *ctx;
    element = gst_rtsp_media_get_element(media);
    appsrc_main = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc_main");
    appsrc_pip = gst_bin_get_by_name_recurse_up(GST_BIN (element), "mysrc_pip");
    timeoverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "timeoverlay");
    infooverlay = gst_bin_get_by_name_recurse_up(GST_BIN (element), "infooverlay");
    videobox = gst_bin_get_by_name_recurse_up(GST_BIN (element), "videomixer");
    videobox_pip = gst_bin_get_by_name_recurse_up(GST_BIN (element), "videobox_pip");
    gst_util_set_object_arg(G_OBJECT (appsrc_main), "format", "time");
    gst_util_set_object_arg(G_OBJECT (appsrc_pip), "format", "time");
    g_object_set(G_OBJECT (appsrc_main), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGBA",
                                     "width", G_TYPE_INT, DualSpecRtspServer::rtspWidth,
                                     "height", G_TYPE_INT, DualSpecRtspServer::rtspHeight,
                                     "framerate", GST_TYPE_FRACTION, fps, 1, NULL), NULL);
    g_object_set(G_OBJECT (appsrc_pip), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGBA",
                                     "width", G_TYPE_INT, 200,
                                     "height", G_TYPE_INT, 200,
                                     "framerate", GST_TYPE_FRACTION, fps, 1, NULL), NULL);
    g_object_set(G_OBJECT (timeoverlay), "halignment", "left", "valignment", "top", "xpad", 50, "ypad", 10, NULL);
    g_object_set(G_OBJECT (infooverlay), "halignment", "right", "valignment", "bottom", "xpad", 50, "ypad", 20, NULL);
    g_object_set(G_OBJECT(ctx->videobox), "top", ctx->ypos, "left", ctx->xpos, "right", "bottom", NULL);
    g_object_set(G_OBJECT(ctx->videobox_pip), "top", ctx->ypos, "left", ctx->xpos, "right", "bottom", NULL);
    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    ctx->timeoverlay = timeoverlay;
    ctx->infooverlay = infooverlay;
    ctx->videobox = videobox;
    ctx->videobox_pip = videobox_pip;
    g_object_set_data_full(G_OBJECT (media), "my-extra-data", ctx, (GDestroyNotify) g_free);
    g_signal_connect(appsrc_main, "need-data", (GCallback) need_data, ctx);
    g_signal_connect(appsrc_pip, "need-data", (GCallback) need_data_pip, ctx);
    gst_object_unref(videobox_pip);
    gst_object_unref(videobox);
    gst_object_unref(infooverlay);
    gst_object_unref(timeoverlay);
    gst_object_unref(appsrc_pip);
    gst_object_unref(appsrc_main);
    gst_object_unref(element);
}
void DualSpecRtspServer::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data){
    setmedia_configure(factory, media, user_data, 1);
}
