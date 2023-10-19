#ifndef RTSPINTERFACE_H
#define RTSPINTERFACE_H

#include <string>
#include <opencv2/opencv.hpp>
#include "RTSPParamInterface.h"

class RTSPInterface : public RTSPParamInterface{
public:
    RTSPInterface(){}
	virtual ~RTSPInterface(){}

public:
    virtual std::string setResolution(int resolution) = 0;
    virtual int getResolution() = 0;

    virtual std::string setName(std::string name) = 0;
    virtual std::string getName() = 0;

    virtual std::string setPushMode(std::string pushmode) = 0;
    virtual std::string getPushMode() = 0;

    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual bool pause() = 0;

public:
    std::string m_name;
    std::string m_resolution;
    std::string m_fusionMode;
    std::string m_pushMode;

};


#endif // RTSPINTERFACE_H