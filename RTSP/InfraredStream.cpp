#include "InfraredStream.h"


InfraredStream::InfraredStream(){

}

InfraredStream::~InfraredStream(){

}

bool InfraredStream::init(){

}

bool InfraredStream::start(){
    isClose = false;
    isRunning = true;
    createSubThread();
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

bool InfraredStream::pause(){
    isRunning = !pausing;
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

string InfraredStream::setResolution(int resolution){
    m_resolution = resolution;
}

int InfraredStream::getResolution(){
    return m_resolution;
}

string InfraredStream::setName(string name){
    m_name = name;
}

string InfraredStream::getName(){
    return m_name;
}

