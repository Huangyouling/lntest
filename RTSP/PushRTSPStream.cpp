#include <string>
#include "PushRTSPStream.h"


PushRTSPStream::PushRTSPStream(CameraConfig config){
    m_cameraConfig = config;
}

PushRTSPStream::~PushRTSPStream(){

}

bool PushRTSPStream::init(){
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    gst_init (&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    server = gst_rtsp_server_new();
    mounts = gst_rtsp_server_get_mount_points(server);

    factory = gst_rtsp_media_factory_new ();
    gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! textoverlay name=timeoverlay halignment=left valignment=top ! textoverlay name=infooverlay halignment=right valignment=bottom ! nvvidconv ! nvv4l2h264enc profile=4 maxperf-enable=1 ! rtph264pay name=pay0 pt=96 )");
    g_signal_connect(factory, "media-configure", (GCallback) media_configure, NULL);
    gst_rtsp_mount_points_add_factory(mounts, "/rgb", factory);
    g_object_unref(mounts);

    gst_rtsp_server_attach(server, NULL);

    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

bool PushRTSPStream::start(){
    isClose = false;
    isRunning = true;
    createSubThread();
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

bool PushRTSPStream::pause(){
    isRunning = !pausing;
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

string PushRTSPStream::setResolution(int resolution){
    m_resolution;
}

int PushRTSPStream::getResolution(){
    return m_resolution;
}

string PushRTSPStream::setName(string name){
    m_name = name;
}

string PushRTSPStream::getName(){
    return m_name;
}

string PushRTSPStream::setFusionMode(string fusionMode){
    m_fusionMode = fusionMode;
}

string PushRTSPStream::getFusionMode(){
    return m_fusionMode;
}

string PushRTSPStream::setPushMode(string pushMode){
    m_pushMode = pushMode;
}

string PushRTSPStream::getPushMode(){
    return m_pushMode;
}



