#ifndef FUSIONSTREAM_H
#define FUSIONSTREAM_H

#include "RTSPInterface.h"

class FusionStream : public RTSPInterface{
public:
    FusionStream();
    virtual ~FusionStream();

public:

    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual bool pause() = 0;

    virtual std::string setResolution(int resolution) = 0;
    virtual int getResolution() = 0;

    virtual std::string setName(std::string name) = 0;
    virtual std::string getName() = 0;

    virtual std::string setPushMode(std::string pushmode) = 0;
    virtual std::string getPushMode() = 0;
    
};


#endif // FUSIONSTREAM_H