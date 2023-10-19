#include "VisibleStream.h"


VisibleStream::VisibleStream(){

}

VisibleStream::~VisibleStream(){

}

bool VisibleStream::init(){

}

bool VisibleStream::start(){
    isClose = false;
    isRunning = true;
    createSubThread();
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

bool VisibleStream::pause(){
    isRunning = !pausing;
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

string VisibleStream::setResolution(int resolution){
    m_resolution = resolution;
}

int VisibleStream::getResolution(){
    return m_resolution;
}

string VisibleStream::setName(string name){
    m_name = name;
}

string VisibleStream::getName(){
    return m_name;
}

