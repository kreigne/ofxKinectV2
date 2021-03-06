//
//  ofxKinectV2.cpp
//  kinectExample
//
//  Created by Theodore Watson on 6/23/14.
//
//

#include "ofxKinectV2.h"

//--------------------------------------------------------------------------------
ofxKinectV2::ofxKinectV2(){
    bNewFrame  = false;
    bNewBuffer = false;
    bOpened    = false;
    lastFrameNo = -1;
    
    //set default distance range to 50cm - 600cm
    
    params.add(minDistance.set("minDistance", 500, 0, 12000));
    params.add(maxDistance.set("maxDistance", 6000, 0, 12000));
}

//--------------------------------------------------------------------------------
ofxKinectV2::~ofxKinectV2(){
    close();
}

//--------------------------------------------------------------------------------
static bool sortBySerialName( const ofxKinectV2::KinectDeviceInfo & A, const ofxKinectV2::KinectDeviceInfo & B ){
    return A.serial < B.serial;
}

//--------------------------------------------------------------------------------
vector <ofxKinectV2::KinectDeviceInfo> ofxKinectV2::getDeviceList(){
    vector <KinectDeviceInfo> devices;
    
    int num = protonect.getFreenect2Instance().enumerateDevices();
    for (int i = 0; i < num; i++){
        KinectDeviceInfo kdi;
        kdi.serial = protonect.getFreenect2Instance().getDeviceSerialNumber(i);
        kdi.freenectId = i; 
        devices.push_back(kdi);
    }
    
    ofSort(devices, sortBySerialName);
    for (int i = 0; i < num; i++){
        devices[i].deviceId = i;
    }
    
    return devices;
}

//--------------------------------------------------------------------------------
unsigned int ofxKinectV2::getNumDevices(){
   return getDeviceList().size(); 
}

//--------------------------------------------------------------------------------
bool ofxKinectV2::open(unsigned int deviceId){
    
    vector <KinectDeviceInfo> devices = getDeviceList();
    
    if( devices.size() == 0 ){
        ofLogError("ofxKinectV2::open") << "no devices connected!";
        return false;
    }
    
    if( deviceId >= devices.size() ){
        ofLogError("ofxKinectV2::open") << " deviceId " << deviceId << " is bigger or equal to the number of connected devices " << devices.size() << endl;
        return false;
    }

    string serial = devices[deviceId].serial;
    return open(serial);
}

//--------------------------------------------------------------------------------
bool ofxKinectV2::open(string serial){
    close(); 
    
    params.setName("kinectV2 " + serial);
    
    bNewFrame  = false;
    bNewBuffer = false;
    bOpened    = false;
    
    int retVal = protonect.openKinect(serial);
    
    if(retVal==0){
        lastFrameNo = -1;
        startThread(true);
        pointCloudFront.resize(512*424);
        pointCloud = pointCloudBack = pointCloudFront;
    }else{
        return false;
    }
    
    bOpened = true;
    return true;
}

//--------------------------------------------------------------------------------
void ofxKinectV2::threadedFunction(){

    while(isThreadRunning()){
        protonect.updateKinect(rgbPixelsBack, depthPixelsBack);
        rgbPixelsFront.swap(rgbPixelsBack);
        depthPixelsFront.swap(depthPixelsBack);
                
        lock();
        pointCloudFront = pointCloudBack;
        bNewBuffer = true;
        unlock();
    }
}

//--------------------------------------------------------------------------------
ofPoint ofxKinectV2::getWorldCoordinateAt(int x, int y, int z){
    int index = x + y * 512;
    libfreenect2::Freenect2Device::IrCameraParams params;
    ofVec3f world;

    if( index >= 0 && index < pointCloud.size() ){
        world = pointCloud[index];
        world.z = z;
        params = protonect.getIrCameraParams();
        world.x = (x - params.cx) * world.z / params.fx;
        world.y = -(y - params.cy) * world.z / params.fy;

        return world;
    }
}

//--------------------------------------------------------------------------------
ofPoint ofxKinectV2::getWorldCoordinateAt(int x, int y){
    int index = x + y * 512;
    libfreenect2::Freenect2Device::IrCameraParams params;
    ofVec3f world;

    if( index >= 0 && index < pointCloud.size() ){
        world = pointCloud[index];
        return getWorldCoordinateAt(x, y, world.z);
    }
}

//--------------------------------------------------------------------------------
vector <ofPoint> ofxKinectV2::getWorldCoordinates(){
    return pointCloud;
}


//--------------------------------------------------------------------------------
void ofxKinectV2::update(){
    if( ofGetFrameNum() != lastFrameNo ){
        bNewFrame = false;
        lastFrameNo = ofGetFrameNum();
    }
    if( bNewBuffer ){
    
        lock();
            rgbPix = rgbPixelsFront;
            rawDepthPixels = depthPixelsFront;
            pointCloud = pointCloudFront;
            bNewBuffer = false;
        unlock();

        bNewFrame = true; 
    }
}


//--------------------------------------------------------------------------------
bool ofxKinectV2::isFrameNew(){
    return bNewFrame; 
}

//--------------------------------------------------------------------------------
ofPixels ofxKinectV2::getDepthPixels(){
    return depthPix;
}

//--------------------------------------------------------------------------------
ofFloatPixels ofxKinectV2::getRawDepthPixels(){
    return rawDepthPixels;
}

//--------------------------------------------------------------------------------
ofPixels ofxKinectV2::getRgbPixels(){
    return rgbPix; 
}

//--------------------------------------------------------------------------------
void ofxKinectV2::close(){
    if( bOpened ){
        waitForThread(true);
        protonect.closeKinect();
        bOpened = false;
    }
}


