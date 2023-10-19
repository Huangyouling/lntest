#include "RTSP.h"
#include "DualSpectrumManager.h"
#include "PushRTSPStream.h"


RTSP::RTSP(){

}

RTSP::~RTSP(){

}

shared_ptr<PushRTSPStream> RTSP::getPushRTSPStream(){
    if(m_PushRTSPStreamPtr == nullptr){
        Log_Error("error: RTSPInterface ptr is nullptr");
    }
    return m_PushRTSPStreamPtr;
}

shared_ptr<DualSpectrumManager> RTSP::getDualSpectrumManager(){
    if(m_DualSpectrumManagerPtr == nullptr){
        Log_Error("error: DualSpectrumManager ptr is nullptr");
    }
    return m_DualSpectrumManagerPtr;
}

void RTSP::openDualCamera(){
    DualSpectrumManager::getInstance()->init();

    m_dualSpectrumCameraPtr = DualSpectrumManager::getInstance()->getDualSpectrumCamera();
    
    m_dualSpectrumCameraPtr->open();

    getImageThread();
}

void RTSP::getImageThread(){
    string errStr;
    m_subthread = std::thread(&RTSPCommont::getDualImage, this, std::ref(errStr));
    m_subthread.detach();
}

void RTSP::getDualImage(string errStr){
    dualSpecImage = m_dualSpectrumCameraPtr->getDualSpacImg(errStr);
}

void RTSP::pushStreamInit(){
    
}


