#ifndef RTSPMANAGER_H
#define RTSPMANAGER_H


#include <iostream>
#include <mutex>
#include "RTSP.h"

class RTSPManager{
public:
    RTSPManager();
    virtual ~RTSPManager();

    static std::shared_ptr<RTSPManager> getInstance();
    std::shared_ptr<RTSP> getRTSP();

    std::string init();
    void uninit();

private:
    static std::shared_ptr<RTSPManager> m_RTSPManagerPtr;
    static std::shared_ptr<mutex> m_mutexPtr;

    std::shared_ptr<RTSP> m_RTSPPtr;

};

#endif // RTSPMANAGER_H