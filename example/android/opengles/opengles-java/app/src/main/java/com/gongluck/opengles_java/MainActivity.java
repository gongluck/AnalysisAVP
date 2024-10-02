package com.gongluck.opengles_java;

import androidx.appcompat.app.AppCompatActivity;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends AppCompatActivity {

    final String TAG = "opengles-java";

    private GLSurfaceView glSurfaceView;
    private boolean rendererSet = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //opengl es版本
        final ActivityManager activityManager = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        Log.i(TAG, "opengl es version: 0x" + Integer.toHexString(configurationInfo.reqGlEsVersion));

        //自定义surfaceview
        glSurfaceView = new GLSurfaceView(this);
        //通知默认的EGLContextFactory和默认的EGLConfigChooser
        //选择哪个EGLContext客户端版本
        glSurfaceView.setEGLContextClientVersion(2);
        //设置渲染器
        glSurfaceView.setRenderer(new OpenGLESRenderer());
        //设置按请求刷新
        //glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        //setContentView(R.layout.activity_main);
        setContentView(glSurfaceView);
    }

    @Override
    protected void onPause() {
        super.onPause();

        if(rendererSet) {
            glSurfaceView.onPause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(rendererSet) {
            glSurfaceView.onResume();
        }
    }
}
