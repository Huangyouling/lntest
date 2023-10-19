// video_test.cpp
#include <iostream>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <time.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <gst/gstelement.h>
#include <gst/gstpipeline.h>
#include <gst/gstutils.h>
#include <gst/app/gstappsrc.h>
#include <gst/base/gstbasesrc.h>
#include <gst/video/video.h>
#include <gst/gst.h>
#include <rtsp-server.h>
#include <glib.h>
#include <pthread.h>
#include <stdlib.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <memory>

cv::Mat img384 = cv::Mat::zeros(cv::Size(1920,1080),CV_8UC3);
std::mutex g_mutex;

int readthreadFun(){

    cv::VideoCapture capture(0);

    if (!capture.isOpened()) {
        std::cout << "Failed to open camera " << std::endl;
    }

    while(true){
        cv::Mat frame;
        capture >> frame;
        if(!frame.empty())
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            img384 = frame.clone();
        }
    }
}

typedef struct
{
    gboolean white;
    GstClockTime timestamp;
} MyContext;

/* called when we need to give data to appsrc */
static void need_data(GstElement * appsrc, guint unused, MyContext * ctx)
{
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;

    size = 1920 * 1080 * 4 + 8;

    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    std::lock_guard<std::mutex> lock(g_mutex);
    memcpy(map.data, img384.data, img384.rows * img384.cols * img384.channels());

    std::cout << "memcpy" << std::endl;

    /* this makes the image black/white */

    ctx->white = !ctx->white;

    /* increment the timestamp every 1/2 second */
    GST_BUFFER_PTS(buffer) = ctx->timestamp;

    int fps = 30;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);
}

/* called when a new media pipeline is constructed. We can query the
* pipeline and configure our appsrc */
static void media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data)
{
    GstElement *element, *appsrc;
    MyContext *ctx;

    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element(media);
    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");
    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT (appsrc), "format", "time");
    int fps = 30;
    /* configure the caps of the video */
    g_object_set(G_OBJECT (appsrc), "caps",
    gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "RGBA",
        "width", G_TYPE_INT, 1920,
        "height", G_TYPE_INT, 1080,
        "framerate", GST_TYPE_FRACTION, fps, 1, NULL), NULL);

    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    /* make sure ther datais freed when the media is gone */
    g_object_set_data_full(G_OBJECT(media), "my-extra-data", ctx, (GDestroyNotify)g_free);
    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", G_CALLBACK (need_data), ctx);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}

int main (int argc, char *argv[])
{

    // DualSpecRtspServer::getInstance()->initRtsp(0, nullptr);
    // return 0;
    auto thread1 = std::thread(&readthreadFun);
    thread1.detach();
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);
    /* create a server instance */
    server = gst_rtsp_server_new();
    /* get the mount points for this server, every server has a default object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points(server);
    /* make a media factory for a test stream. The default media factory can use
    * gst-launch syntax to create pipelines.
    * any launch line works as long as it contains elements named pay%d. Each
    * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new();
    // gst_rtsp_media_factory_set_launch (factory,"( appsrc name=mysrc is-live=true block=true format=GST_FORMAT_TIME !   queue ! nvv4l2h264enc  ! rtph264pay name=pay0 pt=96 )");
    gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! nvvidconv  compute-hw=GPU nvbuf-memory-type=nvbuf-mem-cuda-device !    nvv4l2h264enc profile=4 preset-level=4 MeasureEncoderLatency=1 maxperf-enable=1 ! rtph264pay name=pay0 pt=96 )");
    //  gst_rtsp_media_factory_set_launch (factory,"( appsrc name=mysrc ! nvvidconv !    nvv4l2h264enc ! rtph264pay name=pay0 pt=96 )");
    /* notify when our media is ready, This is called whenever someone asks for
    * the media and a new pipeline with our appsrc is created */
    g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), NULL);
    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory (mounts, "/test", factory);

    /* don't need the ref to the mounts anymore */
    g_object_unref(mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);

    /* start serving */
    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return 0;
}
