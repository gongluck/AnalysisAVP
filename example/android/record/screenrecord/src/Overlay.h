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

#ifndef SCREENRECORD_OVERLAY_H
#define SCREENRECORD_OVERLAY_H

#include "Program.h"
#include "TextRenderer.h"
#include "EglWindow.h"

#include <gui/BufferQueue.h>
#include <gui/GLConsumer.h>
#include <utils/Thread.h>

#include <EGL/egl.h>

namespace android {

/*
 * Overlay "filter".  This sits between the virtual display and the video
 * encoder.
 *
 * Most functions run on a thread created by start().
 */
class Overlay : public GLConsumer::FrameAvailableListener, Thread {
public:
    Overlay(bool monotonicTimestamps) : Thread(false),
        mThreadResult(UNKNOWN_ERROR),
        mState(UNINITIALIZED),
        mFrameAvailable(false),
        mExtTextureName(0),
        mStartMonotonicNsecs(0),
        mStartRealtimeNsecs(0),
        mLastFrameNumber(-1),
        mTotalDroppedFrames(0),
        mUseMonotonicTimestamps(monotonicTimestamps)
        {}

    // Creates a thread that performs the overlay.  Pass in the surface that
    // output will be sent to.
    //
    // This creates a dedicated thread for processing frames.
    //
    // Returns a reference to the producer side of a new BufferQueue that will
    // be used by the virtual display.
    status_t start(const sp<IGraphicBufferProducer>& outputSurface,
            sp<IGraphicBufferProducer>* pBufferProducer);

    // Stops the thread and releases resources.  It's okay to call this even
    // if start() was never called.
    status_t stop();

    // This creates an EGL context and window surface, draws some informative
    // text on it, swaps the buffer, and then tears the whole thing down.
    static status_t drawInfoPage(const sp<IGraphicBufferProducer>& outputSurface);

private:
    Overlay(const Overlay&);
    Overlay& operator=(const Overlay&);

    // Destruction via RefBase.
    virtual ~Overlay() { assert(mState == UNINITIALIZED || mState == STOPPED); }

    // Draw the initial info screen.
    static void doDrawInfoPage(const EglWindow& window,
            const Program& texRender, TextRenderer& textRenderer);

    // (overrides GLConsumer::FrameAvailableListener method)
    virtual void onFrameAvailable(const BufferItem& item);

    // (overrides Thread method)
    virtual bool threadLoop();

    // One-time setup (essentially object construction on the overlay thread).
    status_t setup_l();

    // Release all resources held.
    void release_l();

    // Release EGL display, context, surface.
    void eglRelease_l();

    // Process a frame received from the virtual display.
    void processFrame_l();

    // Convert a monotonic time stamp into a string with the current time.
    void getTimeString_l(nsecs_t monotonicNsec, char* buf, size_t bufLen);

    // Guards all fields below.
    Mutex mMutex;

    // Initialization gate.
    Condition mStartCond;

    // Thread status, mostly useful during startup.
    status_t mThreadResult;

    // Overlay thread state.  States advance from left to right; object may
    // not be restarted.
    enum { UNINITIALIZED, INIT, RUNNING, STOPPING, STOPPED } mState;

    // Event notification.  Overlay thread sleeps on this until a frame
    // arrives or it's time to shut down.
    Condition mEventCond;

    // Set by the FrameAvailableListener callback.
    bool mFrameAvailable;

    // The surface we send our output to, i.e. the video encoder's input
    // surface.
    sp<IGraphicBufferProducer> mOutputSurface;

    // Producer side of queue, passed into the virtual display.
    // The consumer end feeds into our GLConsumer.
    sp<IGraphicBufferProducer> mProducer;

    // This receives frames from the virtual display and makes them available
    // as an external texture.
    sp<GLConsumer> mGlConsumer;

    // EGL display / context / surface.
    EglWindow mEglWindow;

    // GL rendering support.
    Program mExtTexProgram;
    Program mTexProgram;

    // Text rendering.
    TextRenderer mTextRenderer;

    // External texture, updated by GLConsumer.
    GLuint mExtTextureName;

    // Start time, used to map monotonic to wall-clock time.
    nsecs_t mStartMonotonicNsecs;
    nsecs_t mStartRealtimeNsecs;

    // Used for tracking dropped frames.
    nsecs_t mLastFrameNumber;
    size_t mTotalDroppedFrames;

    bool mUseMonotonicTimestamps;

    static const char* kPropertyNames[];
};

}; // namespace android

#endif /*SCREENRECORD_OVERLAY_H*/
