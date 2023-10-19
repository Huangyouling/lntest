#ifndef RTSP_H
#define RTSP_H

#include <opencv2/opencv.hpp>
#include "RTSPInterface.h"
#include "DualSpactrumManager.h"

class RTSP{
public:
    RTSP();
    virtual ~RTSP();
    cv::Mat getImage();
    void pushStream();

    std::shared_ptr<PushRTSPStream> getPushRTSPStream();
    std::shared_ptr<DualSpectrumManager> getDualSpectrumManager();

private:
    static std::shared_ptr<PushRTSPStream> m_PushRTSPStreamPtr;
    static std::shared_ptr<DualSpectrumManager> m_DualSpectrumManagerPtr;

};


#endif // RTSP_H