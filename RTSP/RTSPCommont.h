#ifndef RTSPCOMMONT_h
#define RTSPCOMMONT_h

#include <gst/gstelement.h>
#include <gst/gstpipeline.h>
#include <gst/gstutils.h>
#include <gst/app/gstappsrc.h>
#include <gst/base/gstbasesrc.h>
#include <gst/video/video.h>
#include <gst/gst.h>
#include <rtsp-server.h>
#include <glib.h>


typedef struct{
    gboolean white;
    GstClockTime timestamp;
    GstElement *timeoverlay;
    GstElement *infooverlay;
} MyContext;


class RTSPCommont{
public:
    RTSPCommont();
    virtual ~RTSPCommont();

    void init(string pushMode);
    void media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data);
    void need_data(GstElement * appsrc, guint unused, MyContext * ctx);

    string getCurrentDateTime();
    string getDevice();

public:
    

    DualSpectrumImage dualSpecImage;
    shared_ptr<DualSpectrumCameraDriverInterface> m_dualSpectrumCameraPtr;
};


#endif // RTSPCOMMONT_h