/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ScreenRecord"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#define EGL_EGLEXT_PROTOTYPES

#include <gui/BufferQueue.h>
#include <gui/Surface.h>

#include "EglWindow.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <assert.h>

using namespace android;


status_t EglWindow::createWindow(const sp<IGraphicBufferProducer>& surface) {
    if (mEglSurface != EGL_NO_SURFACE) {
        ALOGE("surface already created");
        return UNKNOWN_ERROR;
    }
    status_t err = eglSetupContext(false);
    if (err != NO_ERROR) {
        return err;
    }

    // Cache the current dimensions.  We're not expecting these to change.
    surface->query(NATIVE_WINDOW_WIDTH, &mWidth);
    surface->query(NATIVE_WINDOW_HEIGHT, &mHeight);

    // Output side (EGL surface to draw on).
    sp<ANativeWindow> anw = new Surface(surface);
    mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, anw.get(),
            NULL);
    if (mEglSurface == EGL_NO_SURFACE) {
        ALOGE("eglCreateWindowSurface error: %#x", eglGetError());
        eglRelease();
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t EglWindow::createPbuffer(int width, int height) {
    if (mEglSurface != EGL_NO_SURFACE) {
        ALOGE("surface already created");
        return UNKNOWN_ERROR;
    }
    status_t err = eglSetupContext(true);
    if (err != NO_ERROR) {
        return err;
    }

    mWidth = width;
    mHeight = height;

    EGLint pbufferAttribs[] = {
            EGL_WIDTH, width,
            EGL_HEIGHT, height,
            EGL_NONE
    };
    mEglSurface = eglCreatePbufferSurface(mEglDisplay, mEglConfig, pbufferAttribs);
    if (mEglSurface == EGL_NO_SURFACE) {
        ALOGE("eglCreatePbufferSurface error: %#x", eglGetError());
        eglRelease();
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t EglWindow::makeCurrent() const {
    if (!eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
        ALOGE("eglMakeCurrent failed: %#x", eglGetError());
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t EglWindow::eglSetupContext(bool forPbuffer) {
    EGLBoolean result;

    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
        ALOGE("eglGetDisplay failed: %#x", eglGetError());
        return UNKNOWN_ERROR;
    }

    EGLint majorVersion, minorVersion;
    result = eglInitialize(mEglDisplay, &majorVersion, &minorVersion);
    if (result != EGL_TRUE) {
        ALOGE("eglInitialize failed: %#x", eglGetError());
        return UNKNOWN_ERROR;
    }
    ALOGV("Initialized EGL v%d.%d", majorVersion, minorVersion);

    EGLint numConfigs = 0;
    EGLint windowConfigAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RECORDABLE_ANDROID, 1,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            // no alpha
            EGL_NONE
    };
    EGLint pbufferConfigAttribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE
    };
    result = eglChooseConfig(mEglDisplay,
            forPbuffer ? pbufferConfigAttribs : windowConfigAttribs,
            &mEglConfig, 1, &numConfigs);
    if (result != EGL_TRUE) {
        ALOGE("eglChooseConfig error: %#x", eglGetError());
        return UNKNOWN_ERROR;
    }

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    mEglContext = eglCreateContext(mEglDisplay, mEglConfig, EGL_NO_CONTEXT,
            contextAttribs);
    if (mEglContext == EGL_NO_CONTEXT) {
        ALOGE("eglCreateContext error: %#x", eglGetError());
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

void EglWindow::eglRelease() {
    ALOGV("EglWindow::eglRelease");
    if (mEglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                EGL_NO_CONTEXT);

        if (mEglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(mEglDisplay, mEglContext);
        }

        if (mEglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(mEglDisplay, mEglSurface);
        }
    }

    mEglDisplay = EGL_NO_DISPLAY;
    mEglContext = EGL_NO_CONTEXT;
    mEglSurface = EGL_NO_SURFACE;
    mEglConfig = NULL;

    eglReleaseThread();
}

// Sets the presentation time on the current EGL buffer.
void EglWindow::presentationTime(nsecs_t whenNsec) const {
    eglPresentationTimeANDROID(mEglDisplay, mEglSurface, whenNsec);
}

// Swaps the EGL buffer.
void EglWindow::swapBuffers() const {
    eglSwapBuffers(mEglDisplay, mEglSurface);
}
