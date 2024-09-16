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

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#define LOG_TAG "ScreenRecord"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <gui/BufferQueue.h>
#include <gui/Surface.h>
#include <cutils/properties.h>
#include <utils/misc.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "screenrecord.h"
#include "Overlay.h"
#include "TextRenderer.h"

using namespace android;

// System properties to look up and display on the info screen.
const char* Overlay::kPropertyNames[] = {
        "ro.build.description",
        // includes ro.build.id, ro.build.product, ro.build.tags, ro.build.type,
        // and ro.build.version.release
        "ro.product.manufacturer",
        "ro.product.model",
        "ro.board.platform",
        "ro.revision",
        "dalvik.vm.heapgrowthlimit",
        "dalvik.vm.heapsize",
        "persist.sys.dalvik.vm.lib.2",
        //"ro.product.cpu.abi",
        //"ro.bootloader",
        //"this-never-appears!",
};


status_t Overlay::start(const sp<IGraphicBufferProducer>& outputSurface,
        sp<IGraphicBufferProducer>* pBufferProducer) {
    ALOGV("Overlay::start");
    mOutputSurface = outputSurface;

    // Grab the current monotonic time and the current wall-clock time so we
    // can map one to the other.  This allows the overlay counter to advance
    // by the exact delay between frames, but if the wall clock gets adjusted
    // we won't track it, which means we'll gradually go out of sync with the
    // times in logcat.
    mStartMonotonicNsecs = systemTime(CLOCK_MONOTONIC);
    mStartRealtimeNsecs = systemTime(CLOCK_REALTIME);

    Mutex::Autolock _l(mMutex);

    // Start the thread.  Traffic begins immediately.
    run("overlay");

    mState = INIT;
    while (mState == INIT) {
        mStartCond.wait(mMutex);
    }

    if (mThreadResult != NO_ERROR) {
        ALOGE("Failed to start overlay thread: err=%d", mThreadResult);
        return mThreadResult;
    }
    assert(mState == RUNNING);

    ALOGV("Overlay::start successful");
    *pBufferProducer = mProducer;
    return NO_ERROR;
}

status_t Overlay::stop() {
    ALOGV("Overlay::stop");
    Mutex::Autolock _l(mMutex);
    mState = STOPPING;
    mEventCond.signal();
    return NO_ERROR;
}

bool Overlay::threadLoop() {
    Mutex::Autolock _l(mMutex);

    mThreadResult = setup_l();

    if (mThreadResult != NO_ERROR) {
        ALOGW("Aborting overlay thread");
        mState = STOPPED;
        release_l();
        mStartCond.broadcast();
        return false;
    }

    ALOGV("Overlay thread running");
    mState = RUNNING;
    mStartCond.broadcast();

    while (mState == RUNNING) {
        mEventCond.wait(mMutex);
        if (mFrameAvailable) {
            ALOGV("Awake, frame available");
            processFrame_l();
            mFrameAvailable = false;
        } else {
            ALOGV("Awake, frame not available");
        }
    }

    ALOGV("Overlay thread stopping");
    release_l();
    mState = STOPPED;
    return false;       // stop
}

status_t Overlay::setup_l() {
    status_t err;

    err = mEglWindow.createWindow(mOutputSurface);
    if (err != NO_ERROR) {
        return err;
    }
    mEglWindow.makeCurrent();

    int width = mEglWindow.getWidth();
    int height = mEglWindow.getHeight();

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Shaders for rendering from different types of textures.
    err = mTexProgram.setup(Program::PROGRAM_TEXTURE_2D);
    if (err != NO_ERROR) {
        return err;
    }
    err = mExtTexProgram.setup(Program::PROGRAM_EXTERNAL_TEXTURE);
    if (err != NO_ERROR) {
        return err;
    }

    err = mTextRenderer.loadIntoTexture();
    if (err != NO_ERROR) {
        return err;
    }
    mTextRenderer.setScreenSize(width, height);

    // Input side (buffers from virtual display).
    glGenTextures(1, &mExtTextureName);
    if (mExtTextureName == 0) {
        ALOGE("glGenTextures failed: %#x", glGetError());
        return UNKNOWN_ERROR;
    }

    sp<IGraphicBufferConsumer> consumer;
    BufferQueue::createBufferQueue(&mProducer, &consumer);
    mGlConsumer = new GLConsumer(consumer, mExtTextureName,
                GL_TEXTURE_EXTERNAL_OES, true, false);
    mGlConsumer->setName(String8("virtual display"));
    mGlConsumer->setDefaultBufferSize(width, height);
    mProducer->setMaxDequeuedBufferCount(4);
    mGlConsumer->setConsumerUsageBits(GRALLOC_USAGE_HW_TEXTURE);

    mGlConsumer->setFrameAvailableListener(this);

    return NO_ERROR;
}


void Overlay::release_l() {
    ALOGV("Overlay::release_l");
    mOutputSurface.clear();
    mGlConsumer.clear();
    mProducer.clear();

    mTexProgram.release();
    mExtTexProgram.release();
    mEglWindow.release();
}

void Overlay::processFrame_l() {
    float texMatrix[16];

    mGlConsumer->updateTexImage();
    mGlConsumer->getTransformMatrix(texMatrix);
    nsecs_t monotonicNsec = mGlConsumer->getTimestamp();
    nsecs_t frameNumber = mGlConsumer->getFrameNumber();

    if (mLastFrameNumber > 0) {
        mTotalDroppedFrames += size_t(frameNumber - mLastFrameNumber) - 1;
    }
    mLastFrameNumber = frameNumber;

    mTextRenderer.setProportionalScale(35);

    if (false) {  // DEBUG - full blue background
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }

    int width = mEglWindow.getWidth();
    int height = mEglWindow.getHeight();
    if (false) {  // DEBUG - draw inset
        mExtTexProgram.blit(mExtTextureName, texMatrix,
                100, 100, width-200, height-200);
    } else {
        mExtTexProgram.blit(mExtTextureName, texMatrix,
                0, 0, width, height);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (false) {  // DEBUG - show entire font bitmap
        mTexProgram.blit(mTextRenderer.getTextureName(), Program::kIdentity,
                100, 100, width-200, height-200);
    }

    char textBuf[64];
    getTimeString_l(monotonicNsec, textBuf, sizeof(textBuf));
    String8 timeStr(String8::format("%s f=%" PRId64 " (%zd)",
            textBuf, frameNumber, mTotalDroppedFrames));
    mTextRenderer.drawString(mTexProgram, Program::kIdentity, 0, 0, timeStr);

    glDisable(GL_BLEND);

    if (false) {  // DEBUG - add red rectangle in lower-left corner
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, 200, 200);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
    }

    mEglWindow.presentationTime(monotonicNsec);
    mEglWindow.swapBuffers();
}

void Overlay::getTimeString_l(nsecs_t monotonicNsec, char* buf, size_t bufLen) {
    //const char* format = "%m-%d %T";    // matches log output
    const char* format = "%T";
    struct tm tm;

    if (mUseMonotonicTimestamps) {
        snprintf(buf, bufLen, "%" PRId64, monotonicNsec);
        return;
    }

    // localtime/strftime is not the fastest way to do this, but a trivial
    // benchmark suggests that the cost is negligible.
    int64_t realTime = mStartRealtimeNsecs +
            (monotonicNsec - mStartMonotonicNsecs);
    time_t secs = (time_t) (realTime / 1000000000);
    localtime_r(&secs, &tm);
    strftime(buf, bufLen, format, &tm);

    int32_t msec = (int32_t) ((realTime % 1000000000) / 1000000);
    char tmpBuf[5];
    snprintf(tmpBuf, sizeof(tmpBuf), ".%03d", msec);
    strlcat(buf, tmpBuf, bufLen);
}

// Callback; executes on arbitrary thread.
void Overlay::onFrameAvailable(const BufferItem& /* item */) {
    ALOGV("Overlay::onFrameAvailable");
    Mutex::Autolock _l(mMutex);
    mFrameAvailable = true;
    mEventCond.signal();
}


/*static*/ status_t Overlay::drawInfoPage(
        const sp<IGraphicBufferProducer>& outputSurface) {
    status_t err;

    EglWindow window;
    err = window.createWindow(outputSurface);
    if (err != NO_ERROR) {
        return err;
    }
    window.makeCurrent();

    int width = window.getWidth();
    int height = window.getHeight();
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Shaders for rendering.
    Program texProgram;
    err = texProgram.setup(Program::PROGRAM_TEXTURE_2D);
    if (err != NO_ERROR) {
        return err;
    }
    TextRenderer textRenderer;
    err = textRenderer.loadIntoTexture();
    if (err != NO_ERROR) {
        return err;
    }
    textRenderer.setScreenSize(width, height);

    doDrawInfoPage(window, texProgram, textRenderer);

    // Destroy the surface.  This causes a disconnect.
    texProgram.release();
    window.release();

    return NO_ERROR;
}

/*static*/ void Overlay::doDrawInfoPage(const EglWindow& window,
        const Program& texProgram, TextRenderer& textRenderer) {
    const nsecs_t holdTime = 250000000LL;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int width = window.getWidth();
    int height = window.getHeight();

    // Draw a thin border around the screen.  Some players, e.g. browser
    // plugins, make it hard to see where the edges are when the device
    // is using a black background, so this gives the viewer a frame of
    // reference.
    //
    // This is a clumsy way to do it, but we're only doing it for one frame,
    // and it's easier than actually drawing lines.
    const int lineWidth = 4;
    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glScissor(0, 0, width, lineWidth);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(0, height - lineWidth, width, lineWidth);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(0, 0, lineWidth, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glScissor(width - lineWidth, 0, lineWidth, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    textRenderer.setProportionalScale(30);

    float xpos = 0;
    float ypos = 0;
    ypos = textRenderer.drawWrappedString(texProgram, xpos, ypos,
            String8::format("Android screenrecord v%d.%d",
                    kVersionMajor, kVersionMinor));

    // Show date/time
    time_t now = time(0);
    struct tm tm;
    localtime_r(&now, &tm);
    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%a, %d %b %Y %T %z", &tm);
    String8 header("Started ");
    header += timeBuf;
    ypos = textRenderer.drawWrappedString(texProgram, xpos, ypos, header);
    ypos += 8 * textRenderer.getScale();    // slight padding

    // Show selected system property values
    for (int i = 0; i < NELEM(kPropertyNames); i++) {
        char valueBuf[PROPERTY_VALUE_MAX];

        property_get(kPropertyNames[i], valueBuf, "");
        if (valueBuf[0] == '\0') {
            continue;
        }
        String8 str(String8::format("%s: [%s]", kPropertyNames[i], valueBuf));
        ypos = textRenderer.drawWrappedString(texProgram, xpos, ypos, str);
    }
    ypos += 8 * textRenderer.getScale();    // slight padding

    // Show GL info
    String8 glStr("OpenGL: ");
    glStr += (char*) glGetString(GL_VENDOR);
    glStr += " / ";
    glStr += (char*) glGetString(GL_RENDERER);
    glStr += ", ";
    glStr += (char*) glGetString(GL_VERSION);
    ypos = textRenderer.drawWrappedString(texProgram, xpos, ypos, glStr);

    //glDisable(GL_BLEND);

    // Set a presentation time slightly in the past.  This will cause the
    // player to hold the frame on screen.
    window.presentationTime(systemTime(CLOCK_MONOTONIC) - holdTime);
    window.swapBuffers();
}
