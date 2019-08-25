/**
 * Based on: GLRendererStereo.cpp
 * Modified by: Brian Webb (2019)
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 **/

#include "GLRendererMono360.h"
#include "jni.h"
#include "OSDRenderer.h"
#include "VideoRenderer.h"
#include <GLES2/gl2.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLRenderSpherical.h>
#include <TelemetryReceiver.h>
#include <Chronometer.h>
#include <cFiles/telemetry.h>
#include "../SettingsN.h"
#include "../Helper/CPUPriorities.h"

#include "gvr.h"
#include "gvr_types.h"

#define PRINT_LOGS
#define TAG "GLRendererMono360"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)


GLRendererMono360::GLRendererMono360(gvr_context *gvr_context) {
    gvr_api_=gvr::GvrApi::WrapNonOwned(gvr_context);
    mTelemetryReceiver=std::make_shared<TelemetryReceiver>(S_OSD_ParseLTM,
							   S_LTMPort,S_OSD_ParseFRSKY,
							   S_FRSKYPort,
							   S_OSD_ParseMAVLINK,
                                                           S_MAVLINKPort,
							   S_OSD_ParseRSSI,
							   S_RSSIPort);
    mHeadTrackerExtended=make_shared<HeadTrackerExtended>(S_InterpupilaryDistance);
}

GLRendererMono360::~GLRendererMono360() {
}

void GLRendererMono360::placeGLElements(){
    int strokeW=2;
    float scale = 1.0 / 130.0;
    mOSDRenderer->placeGLElementsMono360(WindowW*scale,WindowH*scale,strokeW);
    mVideoRenderer->setWorldPosition(-WindowW*scale,-WindowH*scale,1,WindowW*scale,WindowH*scale);
}

void GLRendererMono360::drawScene() {
    mHeadTrackerExtended->calculateNewHeadPose360(gvr_api_.get(),16);
    WorldMatrices* worldMatrices=mHeadTrackerExtended->getWorldMatrices();
    //update and draw the scene
    glViewport(0, 0, ViewPortW, ViewPortH);
    mVideoRenderer->drawGL(worldMatrices->monoViewTracked,worldMatrices->projection360);
    mOSDRenderer->updateAndDrawElements(worldMatrices->monoView,worldMatrices->monoView,worldMatrices->projectionOSD, false);
}

void GLRendererMono360::calculateMetrics(){
    int64_t ts=getTimeMS();
    mFPSCalculator->tick();
    mTelemetryReceiver->get_other_osd_data()->opengl_fps=mFPSCalculator->getCurrentFPS();
    //calculate and print CPU Frame time every 5 seconds
    if(ts-lastLog>5*1000){
        lastLog=ts;
#ifdef PRINT_LOGS
        CPUFrameTime->printAvg();
#endif
        CPUFrameTime->reset();
    }
}

void GLRendererMono360::OnSurfaceCreated(JNIEnv * env,jobject obj,jint videoTexture, jobject assetManagerJAVA) {
    //start the UDP telemetry receiving thread(s)
    mTelemetryReceiver->startReceiving();
    //Once we have an OpenGL context, we can create our OpenGL world object instances. Note the use of shared btw. unique pointers:
    //If the phone does not preserve the OpenGL context when paused, OnSurfaceCreated might be called multiple times
    mGLRenderColor=std::make_shared<GLRenderColor>(false);
    mGLRenderLine=std::make_shared<GLRenderLine>(false);
    mGLRenderText=std::make_shared<GLRenderText>(false);
    mGLRenderText->loadTextureImage(env, obj,assetManagerJAVA);
    mGLRenderSpherical=std::make_shared<GLRenderSpherical>((GLuint)videoTexture);
    mOSDRenderer=std::make_shared<OSDRenderer>(mTelemetryReceiver.get(), mGLRenderColor.get(),mGLRenderLine.get(), mGLRenderText.get());
    mVideoRenderer=std::make_shared<VideoRenderer>(mGLRenderColor.get(),(GLRenderTextureExternal*)NULL,false,mGLRenderSpherical.get());
    lastLog=getTimeMS();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f,0,0,0.0f);
    gvr_api_->ResumeTracking();
}

void GLRendererMono360::OnSurfaceChanged(int width, int height) {
    WindowW=width;
    WindowH=height;
    ViewPortW=width;
    ViewPortH=height;
    mHeadTrackerExtended->calculateMatrices(((float) ViewPortW)/((float)ViewPortH));
    placeGLElements();
    setCPUPriority(CPU_PRIORITY_GLRENDERER_MONO,TAG);
}

void GLRendererMono360::OnDrawFrame() {
    if(changeSwapColor){
        color++;
        if(color>1){
            glClearColor(0.0f,0.0f,0.0f,0.0f);
            color=0;
        }else{
            glClearColor(1.0f,1.0f,0.0f,0.0f);
        }
    }
    if(videoFormatChanged){
        placeGLElements();
        videoFormatChanged=false;
    }
    calculateMetrics();
    CPUFrameTime->start();
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    drawScene();
    CPUFrameTime->stop();
}

void GLRendererMono360::OnGLSurfaceDestroyed() {
    if(mTelemetryReceiver.get()){
        mTelemetryReceiver->stopReceiving();
    }
    if(mOSDRenderer.get()){
        mOSDRenderer->stop();
    }
    gvr_api_->PauseTracking();
}

void GLRendererMono360::OnVideoRatioChanged(int videoW, int videoH) {
    //LOGV("Native video W:%d H:%d",videoW,videoH);
    videoFormat=((float)videoW)/(float)videoH;
    videoFormatChanged=true;
}

void GLRendererMono360::setVideoDecoderFPS(float fps) {
    if(mTelemetryReceiver.get()){
       mTelemetryReceiver->get_other_osd_data()->decoder_fps=fps;
    }
}

/*void GLRendererMono360::OnPause() {
    if(mTelemetryReceiver.get()){
        mTelemetryReceiver->stopReceiving();
    }
    if(mOSDRenderer.get()){
        mOSDRenderer->stop();
    }
}*/

void GLRendererMono360::setHomeLocation(double latitude, double longitude,double attitude) {
    mTelemetryReceiver->setHome(latitude,longitude,attitude);
}
//----------------------------------------------------JAVA bindings---------------------------------------------------------------

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_fpv_1vr_GLRenderer14_1Mono360_##method_name

inline jlong jptr(GLRendererMono360 *glRendererMono360) {
    return reinterpret_cast<intptr_t>(glRendererMono360);
}

inline GLRendererMono360 *native(jlong ptr) {
    return reinterpret_cast<GLRendererMono360 *>(ptr);
}

extern "C" {

JNI_METHOD(jlong, nativeConstructRendererMono360)
(JNIEnv *env, jclass clazz, jlong native_gvr_api) {
    LOGV("create()");
    return jptr(
            new GLRendererMono360(reinterpret_cast<gvr_context *>(native_gvr_api)));
}

JNI_METHOD(void, nativeDestroyRendererMono360)
(JNIEnv *env, jclass clazz, jlong glRendererMono360) {
    LOGV("delete()");
    delete native(glRendererMono360);
}

JNI_METHOD(void, nativeOnSurfaceCreated)
(JNIEnv *env, jobject obj, jlong glRendererMono360,jint videoTexture,jobject assetManagerJAVA) {
    LOGV("OnSurfaceCreated()");
    native(glRendererMono360)->OnSurfaceCreated(env,obj,videoTexture,assetManagerJAVA);
}

JNI_METHOD(void, nativeOnSurfaceChanged)
(JNIEnv *env, jobject obj, jlong glRendererMono360,jint w,jint h) {
    LOGV("OnSurfaceChanged %dx%d",(int)w,(int)h);
    native(glRendererMono360)->OnSurfaceChanged(w,h);
}

JNI_METHOD(void, nativeOnDrawFrame)
(JNIEnv *env, jobject obj, jlong glRendererMono360) {
    //LOGV("OnDrawFrame()");
    native(glRendererMono360)->OnDrawFrame();
}

JNI_METHOD(void, nativeSetVideoDecoderFPS)
(JNIEnv *env, jobject obj, jlong glRendererMono360,jfloat decFPS) {
    native(glRendererMono360)->setVideoDecoderFPS((float)decFPS);
}

JNI_METHOD(void, nativeOnVideoRatioChanged)
(JNIEnv *env, jobject obj, jlong glRendererMono360,jint videoW,jint videoH) {
    native(glRendererMono360)->OnVideoRatioChanged((int)videoW,(int)videoH);
}

JNI_METHOD(void, nativeOnGLSurfaceDestroyed)
(JNIEnv *env, jobject obj, jlong glRendererMono360) {
    LOGV("nativeOnGLSurfaceDestroyed()");
    native(glRendererMono360)->OnGLSurfaceDestroyed();
}
JNI_METHOD(void, nativeSetHomeLocation)
(JNIEnv *env, jobject obj, jlong glRendererMono360,jdouble latitude,jdouble longitude,jdouble attitude) {
    native(glRendererMono360)->setHomeLocation((double)latitude,(double)longitude,(double)attitude);
    LOGV("setHomeLocation()");
}
}
