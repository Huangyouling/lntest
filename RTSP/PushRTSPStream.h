#ifndef PUSHRTSPSTREAM_H
#define PUSHRTSPSTREAM_H

#include <string>
#include "RTSPInterface.h"


class PushRTSPStream : public RTSPInterface{

public:
    PushRTSPStream();
    virtual ~PushRTSPStream();

    bool init();
    bool start();
    bool pause();


    std::string setResolution(int resolution);
    int getResolution();

    std::string setName(std::string name);
    std::string getName();

    std::string setFusionMode(std::string fusionmode);
    std::string getFusionMode();

    std::string setPushMode(std::string pushmode);
    std::string getPushMode();

    void visibleStream();
    void infraredStream();
    void fusionStream();
    void concatStream();
    void pipStream();

private:
    static shared_ptr<PushRTSPStream> m_PushRTSPStreamPtr;
};



#endif // PUSHRTSPSTREAM_H