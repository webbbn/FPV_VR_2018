/**
 * Based on: GLRendererStereo.h
 * Modified by: Brian Webb (2019)
 *
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 **/
/*****************************************************
 * Renders Video and OSD side by side with OpenGL
 * Rendering with and without SuperSync differs slightly
 ******************************************************/

#ifndef FPV_VR_GLRENDERERMONO360_H
#define FPV_VR_GLRENDERERMONO360_H


#include <Chronometer.h>
#include <glm/mat4x4.hpp>
#include <TelemetryReceiver.h>
#include <OSDRenderer.h>
#include <VideoRenderer.h>
#include <gvr_types.h>
#include <HeadTrackerExtended.h>
#include <FPSCalculator.h>

class GLRendererMono360 {
public:
    /**
     * Create a GLRendererMono360 using a given |gvr_context|.
     * @param gvr_api The (non-owned) gvr_context.
     */
    GLRendererMono360(gvr_context* gvr_context);

    /**
   * Destructor.
   */
    ~GLRendererMono360();

    /**
     * Construct OpenGL world objects.
     * @param assetManagerJAVA: a reference to the AssetManager. This is needed for the Text Atlas Texture
     * Also starts the telemetry/text elements update threads
     */
    void OnSurfaceCreated(JNIEnv * env,jobject obj,jint videoTexture, jobject assetManagerJAVA);

    /**
     * recalculate the place(s) of the OpenGL world objects
     */
    void OnSurfaceChanged(int width,int height);

    /**
    * Draw the Video and transparent OSD scene.
    */
    void OnDrawFrame();

    /**
   * stop the telemetry receiver/text elements update thread(s) if existing.
   */
    void OnGLSurfaceDestroyed();

    /**
    * the video decoder fps will be displayed in the OSD
    */
    void setVideoDecoderFPS(float fps);

    /**
     * The OpenGL renderer might start before the first video frame is available (initialized by default with 1280x720)
     * If the video ratio changes, a recalculation of the scene object positions is required
     * @param videoW Video width, in pixels
     * @param videoH Video height, in pixels
     * The recalculation happens in OnDrawFrame, since it needs a valid OpenGL context
     */
    void OnVideoRatioChanged(int videoW,int videoH);

    /**
     * Pass trough the home location lat,lon,alt. May be called multiple times until we have a high enough accuracy.
     * Is called at least once.
     */
    void setHomeLocation(double latitude, double longitude,double attitude);

    void setWorldPosition(float videoX, float videoY, float videoZ, float videoW,float videoH);

    /**
     * Change the view orientation to the saved home orientation.
     */
    void SetHomeOrientation();

    /**
     * Change the home orientation to the current orientation.
     */
    void GoToHomeOrientation();

private:
    std::unique_ptr<gvr::GvrApi> gvr_api_;
    std::shared_ptr<GLRenderColor> mGLRenderColor= nullptr;
    std::shared_ptr<GLRenderLine> mGLRenderLine= nullptr;
    std::shared_ptr<GLRenderText> mGLRenderText= nullptr;
    std::shared_ptr<GLRenderSpherical> mGLRenderSpherical= nullptr;
    std::shared_ptr<VideoRenderer> mVideoRenderer= nullptr;
    std::shared_ptr<TelemetryReceiver> mTelemetryReceiver= nullptr;
    //TODO these could be unique pointers; but c++11 has no make_unique
    std::shared_ptr<OSDRenderer> mOSDRenderer= nullptr;
    //std::unique_ptr CPUFrameTime=std::unique_ptr<Chronometer>(new Chronometer("CPU FrameTime"));
    std::shared_ptr<Chronometer>CPUFrameTime=make_shared<Chronometer>("CPU FrameTime");
    std::shared_ptr<HeadTrackerExtended>mHeadTrackerExtended= nullptr;
    std::shared_ptr<FPSCalculator>mFPSCalculator=make_shared<FPSCalculator>("OpenGL FPS",2000);

    int ViewPortW=0,ViewPortH=0;
    int WindowW=0,WindowH=0;
    std::atomic<bool> videoFormatChanged;
    float videoFormat=1.77777f;
    int color=0;
    bool changeSwapColor= false;
    int64_t lastLog;
    //const float MAX_Z_DISTANCE=14.0f;

    void calculateMetrics();
    void drawScene();
    void placeGLElements();
};


#endif //FPV_VR_GLRENDERERMONO360_H
