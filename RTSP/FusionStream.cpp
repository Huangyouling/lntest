#include "FusionStream.h"


FusionStream::FusionStream(){

}

FusionStream::~FusionStream(){

}

bool FusionStream::init(){
    

}

bool FusionStream::start(){
    isClose = false;
    isRunning = true;
    createSubThread();
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

bool FusionStream::pause(){
    isRunning = !pausing;
    return DC_DualSpectrumErrorCoder::ERR_CODE_0000_OK;
}

string FusionStream::setResolution(int resolution){
    m_resolution = resolution;
}

int FusionStream::getResolution(){
    return m_resolution;
}

string FusionStream::setName(string name){
    m_name = name;
}

string FusionStream::getName(){
    return m_name;
}

