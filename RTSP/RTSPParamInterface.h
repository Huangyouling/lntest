#ifndef RTSPPARAMINTERFACE_H
#define RTSPPARAMINTERFACE_H

#include <string>


class RTSPParamInterface{

public:
    RTSPParamInterface(){}
	virtual ~RTSPParamInterface(){}

    virtual std::string setResolution(int resolution) = 0;
    virtual int getResolution() = 0;

    virtual std::string setName(std::string name) = 0;
    virtual std::string getName() = 0;


    virtual std::string setPushMode(std::string pushmode) = 0;
    virtual std::string getPushMode() = 0;

public:
    std::string m_name;
    std::string m_resolution;
    std::string m_fusionmode;
    std::string m_pushmode;
};



#endif // RTSPSERVERPARAMINTERFACE_H