/*****************************************************
 * Renders OSD only onto a transparent GL Surface.
 * Blending with video is done via Android (HW) composer.
 * This is much more efficient than rendering via OpenGL texture
 ******************************************************/

#ifndef FPV_VR_GLRENDERERMONO_H
#define FPV_VR_GLRENDERERMONO_H


#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <GLProgramVC.h>
#include <GLProgramText.h>
#include <GLProgramSpherical.cpp>
#include <FPSCalculator.h>
#include <MatricesManager.h>
#include <OSD/OSDRenderer.h>
#include <IVideoFormatChanged.hpp>
#include <Chronometer.h>
#include <Video/VideoRenderer.h>

#include "IGLRenderer.h"


class GLRMono360 : public IGLRenderer{
public:
    explicit GLRMono360(JNIEnv* env,jobject androidContext,TelemetryReceiver& telemetryReceiver,gvr_context* gvr_context);
private:
    void onSurfaceCreated(JNIEnv * env,jobject obj,jint optionalVideoTexture) override;
    void onSurfaceChanged(int width, int height)override;
    //Draw the transparent OSD scene.
    void onDrawFrame()override ;
private:
    TelemetryReceiver& mTelemetryReceiver;
    MatricesManager mMatricesM;
    Chronometer cpuFrameTime;
    FPSCalculator mFPSCalculator;
    const SettingsVR mSettingsVR;
    std::unique_ptr<BasicGLPrograms> mBasicGLPrograms=nullptr;
    std::unique_ptr<OSDRenderer> mOSDRenderer= nullptr;
    std::unique_ptr<GLProgramSpherical> mGLProgramSpherical=nullptr;
    std::unique_ptr<VideoRenderer> mVideoRenderer= nullptr;
    std::unique_ptr<gvr::GvrApi> gvr_api_;
};


#endif //FPV_VR_GLRENDERERMONO_H