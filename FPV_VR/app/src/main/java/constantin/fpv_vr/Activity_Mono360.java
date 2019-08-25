package constantin.fpv_vr;
/**
 * Based on: Activity_Stereo.java
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
/* ************************************************************************
 * Renders Video & OSD Side by Side.
 * Pipeline h.264-->image on screen:
 * h.264 nalus->VideoDecoder->SurfaceTexture-(updateTexImage)->Texture->Rendering with OpenGL
 ***************************************************************************/

import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.PowerManager;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;

import com.google.vr.cardboard.DisplaySynchronizer;
import com.google.vr.ndk.base.GvrApi;
import com.google.vr.ndk.base.GvrLayout;
import com.google.vr.ndk.base.GvrUiLayout;
import com.google.vr.sdk.base.AndroidCompat;
import com.google.vr.sdk.base.Eye;

public class Activity_Mono360 extends AppCompatActivity {
    private GLSurfaceViewEGL14 mGLView14Mono360;
    private GLRenderer14_Mono360 mGLRenderer14Mono360;
    private GvrLayout mGvrLayout;
    private GvrApi mGvrApi;
    private HomeLocation mHomeLocation;

    //We mustn't call onPause/onResume on the Gvr layout, or else the surface might be destroyed/app will crash. TODO: track this issue to it's roots
    private final boolean useGVRLayout=true;

    private void enableSustainedPerformanceIfPossible(){
        if (Build.VERSION.SDK_INT >= 24) {
            final PowerManager powerManager = (PowerManager)getSystemService(POWER_SERVICE);
            if(powerManager!=null){
                if (powerManager.isSustainedPerformanceModeSupported()) {
                    //slightly lower, but sustainable clock speeds
                    //I also enable this mode (if the device supports it) when not doing front buffer rendering,
                    //because when the user decides to render at 120fps or more (disable vsync/60fpsCap)
                    //the App benefits from sustained performance, too
                    AndroidCompat.setSustainedPerformanceMode(this,true);
                    System.out.println("Sustained performance set true");
                }else{
                    System.out.println("Sustained performance not available");
                }
            }
        }
    }
    private void disableSustainedPerformanceIfEnabled(){
        if (Build.VERSION.SDK_INT >= 24) {
            final PowerManager powerManager = (PowerManager)getSystemService(POWER_SERVICE);
            if(powerManager!=null){
                if (powerManager.isSustainedPerformanceModeSupported()) {
                    AndroidCompat.setSustainedPerformanceMode(this, false);
                }
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        System.out.println("OnCreate");
        super.onCreate(savedInstanceState);
        int mode = GLSurfaceViewEGL14.MODE_NORMAL;
        if (Settings.DisableVSYNC) {
            mode = GLSurfaceViewEGL14.MODE_VSYNC_OFF;
        } else if (Settings.Disable60fpsCap) {
            mode = GLSurfaceViewEGL14.MODE_UNLIMITED_FPS_BUT_VSYNC_ON;
        }
/*
        if (useGVRLayout) {
            mGvrLayout = new GvrLayout(this);
            mGvrLayout.setAsyncReprojectionEnabled(false);
            mGLView14Mono360 = new GLSurfaceViewEGL14(this, mode, Settings.MSAALevel);
            mGLRenderer14Mono360 = new GLRenderer14_Mono360(this, mGvrLayout.getGvrApi().getNativeGvrContext());
            mGLView14Mono360.setRenderer(mGLRenderer14Mono360);
            mGLView14Mono360.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
            mGLView14Mono360.setPreserveEGLContextOnPause(false);
            mGvrLayout.setPresentationView(mGLView14Mono360);
            mGvrLayout.getUiLayout().setSettingsButtonEnabled(false);
            setContentView(mGvrLayout);
        } else {
*/
            mGvrApi = new GvrApi(this, new DisplaySynchronizer(this, getWindowManager().getDefaultDisplay()));
            mGvrApi.reconnectSensors();
            mGvrApi.usingVrDisplayService();
            mGvrApi.clearError();
            mGLView14Mono360 = new GLSurfaceViewEGL14(this, mode, Settings.MSAALevel);
            mGLRenderer14Mono360 = new GLRenderer14_Mono360(this, mGvrApi.getNativeGvrContext());
            mGLView14Mono360.setRenderer(mGLRenderer14Mono360);
            mGLView14Mono360.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
            mGLView14Mono360.setPreserveEGLContextOnPause(false);
            setContentView(mGLView14Mono360);
//        }
        mHomeLocation=new HomeLocation(this,mGLRenderer14Mono360);
    }

    @Override
    protected void onResume() {
        super.onResume();
        //Hide the Action Bar & go into immersive fullscreen sticky mode
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        enableSustainedPerformanceIfPossible();
        //if(useGVRLayout){
        //    mGvrLayout.onResume();
        //}
        mGLView14Mono360.onResume();
        mHomeLocation.resume();
    }

    @Override
    protected void onPause(){
        super.onPause();
        //if(useGVRLayout){
            //calling onResume/onPause on the GvrLayou results in some weird onPause/onSurfaceDestroyed
            //mGvrLayout.onPause();
        //}
        mGLView14Mono360.onPause();
        disableSustainedPerformanceIfEnabled();
        mHomeLocation.pause();
    }

    @Override
    protected void onDestroy(){
        super.onDestroy();
        //mGLRenderer14Mono360.onDelete();
        if(useGVRLayout){
            mGvrLayout.shutdown();
        }else{
            mGvrApi.shutdown();
        }
        mGvrLayout=null;
        mGLView14Mono360=null;
        mGLRenderer14Mono360=null;
    }
}
