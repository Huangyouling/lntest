#include <mutex>
#include "RTSPManager.h"
#include "RTSP.h"


shared_ptr<RTSPManager> RTSPManager::m_RTSPManagerPtr = shared_ptr<RTSPManager>();
shared_ptr<mutex> RTSPManager::m_mutexPtr = shared_ptr<mutex>(new mutex);

RTSPManager::RTSPManager(){
    m_RTSPPtr.reset(new RTSP);
}

RTSPManager::~RTSPManager(){

}

string RTSPManager::init(){
    shared_ptr<RTSP> RTSPPtr = RTSPManager::getInstance()->getRTSP();
    RTSPPtr->getPushRTSPStream();
    RTSPPtr->getDualSpectrumManager();

}

void RTSPManager::uninit(){

}

shared_ptr<RTSPManager> RTSPManager::getInstance(){
    if (m_RTSPManagerPtr == nullptr){
        lock_guard<mutex> lock(*m_mutexPtr);

        if (m_RTSPManagerPtr == nullptr){
            m_RTSPManagerPtr.reset(new RTSPManager);
        }
    }
    return m_RTSPManagerPtr;
}

shared_ptr<RTSP> RTSPManager::getRTSP(){
    if(m_RTSPPtr == nullptr){
        Log_Error("error: RTSP error");
    }
    return m_RTSPPtr;
}


