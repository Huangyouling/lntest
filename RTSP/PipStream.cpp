#include "PipStream.h"


PipStream::PipStream(){

}

PipStream::~PipStream(){

}

bool PipStream::init(){

}

bool PipStream::start(){
    isClose = false;
    isRunning = true;
    createSubThread();
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

bool PipStream::pause(){
    isRunning = !pausing;
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

string PipStream::setResolution(int resolution){
    m_resolution = resolution;
}

int PipStream::getResolution(){
    return m_resolution;
}

string PipStream::setName(string name){
    m_name = name;
}

string PipStream::getName(){
    return m_name;
}

