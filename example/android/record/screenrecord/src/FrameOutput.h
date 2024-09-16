/*
 * Copyright 2014 The Android Open Source Project
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

#ifndef SCREENRECORD_FRAMEOUTPUT_H
#define SCREENRECORD_FRAMEOUTPUT_H

#include "Program.h"
#include "EglWindow.h"

#include <gui/BufferQueue.h>
#include <gui/GLConsumer.h>

namespace android {

/*
 * Support for "frames" output format.
 */
class FrameOutput : public GLConsumer::FrameAvailableListener {
public:
    FrameOutput() : mFrameAvailable(false),
        mExtTextureName(0),
        mPixelBuf(NULL)
        {}

    // Create an "input surface", similar in purpose to a MediaCodec input
    // surface, that the virtual display can send buffers to.  Also configures
    // EGL with a pbuffer surface on the current thread.
    status_t createInputSurface(int width, int height,
            sp<IGraphicBufferProducer>* pBufferProducer);

    // Copy one from input to output.  If no frame is available, this will wait up to the
    // specified number of microseconds.
    //
    // Returns ETIMEDOUT if the timeout expired before we found a frame.
    status_t copyFrame(FILE* fp, long timeoutUsec, bool rawFrames);

    // Prepare to copy frames.  Makes the EGL context used by this object current.
    void prepareToCopy() {
        mEglWindow.makeCurrent();
    }

private:
    FrameOutput(const FrameOutput&);
    FrameOutput& operator=(const FrameOutput&);

    // Destruction via RefBase.
    virtual ~FrameOutput() {
        delete[] mPixelBuf;
    }

    // (overrides GLConsumer::FrameAvailableListener method)
    virtual void onFrameAvailable(const BufferItem& item);

    // Reduces RGBA to RGB, in place.
    static void reduceRgbaToRgb(uint8_t* buf, unsigned int pixelCount);

    // Put a 32-bit value into a buffer, in little-endian byte order.
    static void setValueLE(uint8_t* buf, uint32_t value);

    // Used to wait for the FrameAvailableListener callback.
    Mutex mMutex;
    Condition mEventCond;

    // Set by the FrameAvailableListener callback.
    bool mFrameAvailable;

    // This receives frames from the virtual display and makes them available
    // as an external texture.
    sp<GLConsumer> mGlConsumer;

    // EGL display / context / surface.
    EglWindow mEglWindow;

    // GL rendering support.
    Program mExtTexProgram;

    // External texture, updated by GLConsumer.
    GLuint mExtTextureName;

    // Pixel data buffer.
    uint8_t* mPixelBuf;
};

}; // namespace android

#endif /*SCREENRECORD_FRAMEOUTPUT_H*/
