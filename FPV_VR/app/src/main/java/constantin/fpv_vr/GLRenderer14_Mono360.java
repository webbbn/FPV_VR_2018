/**
 * Based on: GLRenderer14_Stereo.java
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
package constantin.fpv_vr;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.location.Location;
import android.opengl.EGL14;
import android.opengl.EGLExt;
import android.opengl.GLES20;
import android.view.Surface;
import com.google.vr.ndk.base.GvrApi;

/**
 * Description: see Activity_360
 */

public class GLRenderer14_Mono360 implements GLSurfaceViewEGL14.IRendererEGL14,VideoPlayer.VideoParamsChanged,HomeLocation.HomeLocationChanged{
    static {
        System.loadLibrary("GLRendererMono360");
    }
    // Opaque native pointer to the native GLRendererMono360 instance.
    private long nativeGLRendererMono360;
    private static native long nativeConstructRendererMono360(long nativeGvrContext);
    private static native long nativeDestroyRendererMono360(long glRendererMono360P);
    //surfaceCreated->surfaceChanged->drawFrame->GLSurfaceDestroyed->repeat
    public static native void nativeOnSurfaceCreated(long glRendererMono360P,int videoTexture,AssetManager assetManager);
    public static native void nativeOnSurfaceChanged(long glRendererMono360P,int width,int height);
    public static native void nativeOnDrawFrame(long glRendererMono360P);
    private static native void nativeSetVideoDecoderFPS(long glRendererMono360P,float fps);
    private static native void nativeOnVideoRatioChanged(long glRendererMono360P,int videoW,int videoH);
    //since we do not preserve the OpenGL context when paused, nativeOnSurfaceCreated acts as a "onResume" equivalent.
    //private static native void nativeOnPause(long glRendererMono360P);
    private static native void nativeOnGLSurfaceDestroyed(long glRendererMono360P);
    private static native void nativeSetHomeLocation(long glRendererMono360P,double latitude, double longitude,double attitude);
    private static native void nativeSetHomeOrientation(long glRendererMono360P);
    private static native void nativeGoToHomeOrientation(long glRendererMono360P);

    private final Context mContext;
    private SurfaceTexture mSurfaceTexture=null;
    private VideoPlayer mVideoPlayer=null;


    public GLRenderer14_Mono360(Context activityContext, long gvrApiNativeContext){
        mContext=activityContext;
        nativeGLRendererMono360=nativeConstructRendererMono360(
                gvrApiNativeContext);
    }

    @Override
    public void onSurfaceCreated() {
        int[] videoTexture=new int[1];
        GLES20.glGenTextures(1, videoTexture, 0);
        mSurfaceTexture = new SurfaceTexture(videoTexture[0],false);
        Surface mVideoSurface=new Surface(mSurfaceTexture);
        mVideoPlayer=VideoPlayer.createAndStartVideoPlayer(mContext,mVideoSurface,this);
        AssetManager assetManager=mContext.getAssets();
        nativeOnSurfaceCreated(nativeGLRendererMono360,videoTexture[0],assetManager);
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        nativeOnSurfaceChanged(nativeGLRendererMono360,width,height);
    }

    @Override
    public void onDrawFrame() {
        if(Settings.Disable60fpsCap){
            EGLExt.eglPresentationTimeANDROID(EGL14.eglGetCurrentDisplay(),EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW),System.nanoTime());
        }
        mSurfaceTexture.updateTexImage();
        if(Settings.Disable60fpsCap){
            EGLExt.eglPresentationTimeANDROID(EGL14.eglGetCurrentDisplay(),EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW),System.nanoTime());
        }
        nativeOnDrawFrame(nativeGLRendererMono360);
    }

    @Override
    public void onGLSurfaceDestroyed() {
        //System.out.println("GL14 onDestroy");
        if(mVideoPlayer!=null){
            VideoPlayer.stopAndDeleteVideoPlayer(mVideoPlayer);
            mVideoPlayer=null;
        }
        if(mSurfaceTexture!=null){
            mSurfaceTexture.release();
            mSurfaceTexture=null;
        }
        nativeOnGLSurfaceDestroyed(nativeGLRendererMono360);
    }

    @Override
    public void onVideoRatioChanged(int videoW, int videoH) {
        nativeOnVideoRatioChanged(nativeGLRendererMono360,videoW,videoH);
    }

    @Override
    public void onVideoFPSChanged(float decFPS) {
        nativeSetVideoDecoderFPS(nativeGLRendererMono360,decFPS);
    }

    @Override
    protected void finalize() throws Throwable {
        //System.out.println("Hi finalize");
        //TODO: is this a good practice ? maybe take a deeper look. But it 'works'
        try {
            nativeDestroyRendererMono360(nativeGLRendererMono360);
        } finally {
            super.finalize();
        }
    }


    @Override
    public void onHomeLocationChanged(Location location) {
        nativeSetHomeLocation(nativeGLRendererMono360,location.getLatitude(),location.getLongitude(),location.getAltitude());
    }

    public void setHomeOrientation() {
        nativeSetHomeOrientation(nativeGLRendererMono360);
    }

    public void goToHomeOrientation() {
        nativeGoToHomeOrientation(nativeGLRendererMono360);
    }
}
