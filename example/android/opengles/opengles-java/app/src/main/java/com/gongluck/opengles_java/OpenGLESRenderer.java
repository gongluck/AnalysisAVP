package com.gongluck.opengles_java;

import android.opengl.GLSurfaceView;

import java.util.Random;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static android.opengl.GLES20.*;

public class OpenGLESRenderer implements GLSurfaceView.Renderer {

    private Random random = new Random();
    private float getRandom() {
        return random.nextInt(100) / 100.0f;
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        //glClearColor(getRandom(), getRandom(), getRandom(), getRandom());
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int width, int height) {
        glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        glClearColor(getRandom(), getRandom(), getRandom(), getRandom());
        glClear(GL_COLOR_BUFFER_BIT);
    }
}
