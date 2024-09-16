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

// https://gongluck.github.io/av/screenrecord
// from: /platform/framework/av/cmds/screenrecord/screenrecord.cpp

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <termios.h>
#include <unistd.h>

#define LOG_TAG "ScreenRecord"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <utils/Errors.h>
#include <utils/Timers.h>
#include <utils/Trace.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaCodecConstants.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaMuxer.h>
#include <media/stagefright/PersistentSurface.h>
#include <media/ICrypto.h>
#include <media/MediaCodecBuffer.h>

#include "screenrecord.h"
#include "Overlay.h"
#include "FrameOutput.h"

using android::ABuffer;
using android::ALooper;
using android::AMessage;
using android::AString;
using android::DisplayInfo;
using android::FrameOutput;
using android::IBinder;
using android::IGraphicBufferProducer;
using android::ISurfaceComposer;
using android::MediaCodec;
using android::MediaCodecBuffer;
using android::MediaMuxer;
using android::Overlay;
using android::PersistentSurface;
using android::ProcessState;
using android::Rect;
using android::String8;
using android::SurfaceComposerClient;
using android::Vector;
using android::sp;
using android::status_t;

using android::DISPLAY_ORIENTATION_0;
using android::DISPLAY_ORIENTATION_180;
using android::DISPLAY_ORIENTATION_90;
using android::INVALID_OPERATION;
using android::NAME_NOT_FOUND;
using android::NO_ERROR;
using android::UNKNOWN_ERROR;

static const uint32_t kMinBitRate = 100000;         // 0.1Mbps
static const uint32_t kMaxBitRate = 200 * 1000000;  // 200Mbps
static const uint32_t kMaxTimeLimitSec = 180;       // 3 minutes
static const uint32_t kFallbackWidth = 1280;        // 720p
static const uint32_t kFallbackHeight = 720;
static const char* kMimeTypeAvc = "video/avc";

// Command-line parameters.
static bool gVerbose = false;           // chatty on stdout
static bool gRotate = false;            // rotate 90 degrees
static bool gMonotonicTime = false;     // use system monotonic time for timestamps
static bool gPersistentSurface = false; // use persistent surface
static enum {
    FORMAT_MP4, FORMAT_H264, FORMAT_WEBM, FORMAT_3GPP, FORMAT_FRAMES, FORMAT_RAW_FRAMES
} gOutputFormat = FORMAT_MP4;           // data format for output
static AString gCodecName = "";         // codec name override
static bool gSizeSpecified = false;     // was size explicitly requested?
static bool gWantInfoScreen = false;    // do we want initial info screen?
static bool gWantFrameTime = false;     // do we want times on each frame?
static uint32_t gVideoWidth = 0;        // default width+height
static uint32_t gVideoHeight = 0;
static uint32_t gBitRate = 20000000;     // 20Mbps
static uint32_t gTimeLimitSec = kMaxTimeLimitSec;
static uint32_t gBframes = 0;

// Set by signal handler to stop recording.
static volatile bool gStopRequested = false;

// Previous signal handler state, restored after first hit.
static struct sigaction gOrigSigactionINT;
static struct sigaction gOrigSigactionHUP;


/*
 * Catch keyboard interrupt signals.  On receipt, the "stop requested"
 * flag is raised, and the original handler is restored (so that, if
 * we get stuck finishing, a second Ctrl-C will kill the process).
 */
static void signalCatcher(int signum)
{
    gStopRequested = true;
    switch (signum) {
    case SIGINT:
    case SIGHUP:
        sigaction(SIGINT, &gOrigSigactionINT, NULL);
        sigaction(SIGHUP, &gOrigSigactionHUP, NULL);
        break;
    default:
        abort();
        break;
    }
}

/*
 * Configures signal handlers.  The previous handlers are saved.
 *
 * If the command is run from an interactive adb shell, we get SIGINT
 * when Ctrl-C is hit.  If we're run from the host, the local adb process
 * gets the signal, and we get a SIGHUP when the terminal disconnects.
 */
static status_t configureSignals() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalCatcher;
    if (sigaction(SIGINT, &act, &gOrigSigactionINT) != 0) {
        status_t err = -errno;
        fprintf(stderr, "Unable to configure SIGINT handler: %s\n",
                strerror(errno));
        return err;
    }
    if (sigaction(SIGHUP, &act, &gOrigSigactionHUP) != 0) {
        status_t err = -errno;
        fprintf(stderr, "Unable to configure SIGHUP handler: %s\n",
                strerror(errno));
        return err;
    }
    signal(SIGPIPE, SIG_IGN);
    return NO_ERROR;
}

/*
 * Configures and starts the MediaCodec encoder.  Obtains an input surface
 * from the codec.
 */
static status_t prepareEncoder(float displayFps, sp<MediaCodec>* pCodec,
        sp<IGraphicBufferProducer>* pBufferProducer) {
    status_t err;

    if (gVerbose) {
        printf("Configuring recorder for %dx%d %s at %.2fMbps\n",
                gVideoWidth, gVideoHeight, kMimeTypeAvc, gBitRate / 1000000.0);
        fflush(stdout);
    }

    sp<AMessage> format = new AMessage;
    format->setInt32(KEY_WIDTH, gVideoWidth);
    format->setInt32(KEY_HEIGHT, gVideoHeight);
    format->setString(KEY_MIME, kMimeTypeAvc);
    format->setInt32(KEY_COLOR_FORMAT, OMX_COLOR_FormatAndroidOpaque);
    format->setInt32(KEY_BIT_RATE, gBitRate);
    format->setFloat(KEY_FRAME_RATE, displayFps);
    format->setInt32(KEY_I_FRAME_INTERVAL, 10);
    format->setInt32(KEY_MAX_B_FRAMES, gBframes);
    if (gBframes > 0) {
        format->setInt32(KEY_PROFILE, AVCProfileMain);
        format->setInt32(KEY_LEVEL, AVCLevel41);
    }

    sp<android::ALooper> looper = new android::ALooper;
    looper->setName("screenrecord_looper");
    looper->start();
    ALOGV("Creating codec");
    sp<MediaCodec> codec;
    if (gCodecName.empty()) {
        codec = MediaCodec::CreateByType(looper, kMimeTypeAvc, true);
        if (codec == NULL) {
            fprintf(stderr, "ERROR: unable to create %s codec instance\n",
                    kMimeTypeAvc);
            return UNKNOWN_ERROR;
        }
    } else {
        codec = MediaCodec::CreateByComponentName(looper, gCodecName);
        if (codec == NULL) {
            fprintf(stderr, "ERROR: unable to create %s codec instance\n",
                    gCodecName.c_str());
            return UNKNOWN_ERROR;
        }
    }

    err = codec->configure(format, NULL, NULL,
            MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != NO_ERROR) {
        fprintf(stderr, "ERROR: unable to configure %s codec at %dx%d (err=%d)\n",
                kMimeTypeAvc, gVideoWidth, gVideoHeight, err);
        codec->release();
        return err;
    }

    ALOGV("Creating encoder input surface");
    sp<IGraphicBufferProducer> bufferProducer;
    if (gPersistentSurface) {
        sp<PersistentSurface> surface = MediaCodec::CreatePersistentInputSurface();
        bufferProducer = surface->getBufferProducer();
        err = codec->setInputSurface(surface);
    } else {
        err = codec->createInputSurface(&bufferProducer);
    }
    if (err != NO_ERROR) {
        fprintf(stderr,
            "ERROR: unable to %s encoder input surface (err=%d)\n",
            gPersistentSurface ? "set" : "create",
            err);
        codec->release();
        return err;
    }

    ALOGV("Starting codec");
    err = codec->start();
    if (err != NO_ERROR) {
        fprintf(stderr, "ERROR: unable to start codec (err=%d)\n", err);
        codec->release();
        return err;
    }

    ALOGV("Codec prepared");
    *pCodec = codec;
    *pBufferProducer = bufferProducer;
    return 0;
}

/*
 * Sets the display projection, based on the display dimensions, video size,
 * and device orientation.
 */
static status_t setDisplayProjection(
        SurfaceComposerClient::Transaction& t,
        const sp<IBinder>& dpy,
        const DisplayInfo& mainDpyInfo) {

    // Set the region of the layer stack we're interested in, which in our
    // case is "all of it".
    Rect layerStackRect(mainDpyInfo.viewportW, mainDpyInfo.viewportH);

    // We need to preserve the aspect ratio of the display.
    float displayAspect = (float) mainDpyInfo.viewportH / (float) mainDpyInfo.viewportW;


    // Set the way we map the output onto the display surface (which will
    // be e.g. 1280x720 for a 720p video).  The rect is interpreted
    // post-rotation, so if the display is rotated 90 degrees we need to
    // "pre-rotate" it by flipping width/height, so that the orientation
    // adjustment changes it back.
    //
    // We might want to encode a portrait display as landscape to use more
    // of the screen real estate.  (If players respect a 90-degree rotation
    // hint, we can essentially get a 720x1280 video instead of 1280x720.)
    // In that case, we swap the configured video width/height and then
    // supply a rotation value to the display projection.
    uint32_t videoWidth, videoHeight;
    uint32_t outWidth, outHeight;
    if (!gRotate) {
        videoWidth = gVideoWidth;
        videoHeight = gVideoHeight;
    } else {
        videoWidth = gVideoHeight;
        videoHeight = gVideoWidth;
    }
    if (videoHeight > (uint32_t)(videoWidth * displayAspect)) {
        // limited by narrow width; reduce height
        outWidth = videoWidth;
        outHeight = (uint32_t)(videoWidth * displayAspect);
    } else {
        // limited by short height; restrict width
        outHeight = videoHeight;
        outWidth = (uint32_t)(videoHeight / displayAspect);
    }
    uint32_t offX, offY;
    offX = (videoWidth - outWidth) / 2;
    offY = (videoHeight - outHeight) / 2;
    Rect displayRect(offX, offY, offX + outWidth, offY + outHeight);

    if (gVerbose) {
        if (gRotate) {
            printf("Rotated content area is %ux%u at offset x=%d y=%d\n",
                    outHeight, outWidth, offY, offX);
            fflush(stdout);
        } else {
            printf("Content area is %ux%u at offset x=%d y=%d\n",
                    outWidth, outHeight, offX, offY);
            fflush(stdout);
        }
    }

    t.setDisplayProjection(dpy,
            gRotate ? DISPLAY_ORIENTATION_90 : DISPLAY_ORIENTATION_0,
            layerStackRect, displayRect);
    return NO_ERROR;
}

/*
 * Configures the virtual display.  When this completes, virtual display
 * frames will start arriving from the buffer producer.
 */
static status_t prepareVirtualDisplay(const DisplayInfo& mainDpyInfo,
        const sp<IGraphicBufferProducer>& bufferProducer,
        sp<IBinder>* pDisplayHandle) {
    sp<IBinder> dpy = SurfaceComposerClient::createDisplay(
            String8("ScreenRecorder"), false /*secure*/);

    SurfaceComposerClient::Transaction t;
    t.setDisplaySurface(dpy, bufferProducer);
    setDisplayProjection(t, dpy, mainDpyInfo);
    t.setDisplayLayerStack(dpy, 0);    // default stack
    t.apply();

    *pDisplayHandle = dpy;

    return NO_ERROR;
}

/*
 * Runs the MediaCodec encoder, sending the output to the MediaMuxer.  The
 * input frames are coming from the virtual display as fast as SurfaceFlinger
 * wants to send them.
 *
 * Exactly one of muxer or rawFp must be non-null.
 *
 * The muxer must *not* have been started before calling.
 */
static status_t runEncoder(const sp<MediaCodec>& encoder,
        const sp<MediaMuxer>& muxer, FILE* rawFp, const sp<IBinder>& mainDpy,
        const sp<IBinder>& virtualDpy, uint8_t orientation) {
    static int kTimeout = 250000;   // be responsive on signal
    status_t err;
    ssize_t trackIdx = -1;
    uint32_t debugNumFrames = 0;
    int64_t startWhenNsec = systemTime(CLOCK_MONOTONIC);
    int64_t endWhenNsec = startWhenNsec + seconds_to_nanoseconds(gTimeLimitSec);
    DisplayInfo mainDpyInfo;

    assert((rawFp == NULL && muxer != NULL) || (rawFp != NULL && muxer == NULL));

    Vector<sp<MediaCodecBuffer> > buffers;
    err = encoder->getOutputBuffers(&buffers);
    if (err != NO_ERROR) {
        fprintf(stderr, "Unable to get output buffers (err=%d)\n", err);
        return err;
    }

    // Run until we're signaled.
    while (!gStopRequested) {
        size_t bufIndex, offset, size;
        int64_t ptsUsec;
        uint32_t flags;

        if (systemTime(CLOCK_MONOTONIC) > endWhenNsec) {
            if (gVerbose) {
                printf("Time limit reached\n");
                fflush(stdout);
            }
            break;
        }

        ALOGV("Calling dequeueOutputBuffer");
        err = encoder->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec,
                &flags, kTimeout);
        ALOGV("dequeueOutputBuffer returned %d", err);
        switch (err) {
        case NO_ERROR:
            // got a buffer
            if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) != 0) {
                ALOGV("Got codec config buffer (%zu bytes)", size);
                if (muxer != NULL) {
                    // ignore this -- we passed the CSD into MediaMuxer when
                    // we got the format change notification
                    size = 0;
                }
            }
            if (size != 0) {
                ALOGV("Got data in buffer %zu, size=%zu, pts=%" PRId64,
                        bufIndex, size, ptsUsec);

                { // scope
                    ATRACE_NAME("orientation");
                    // Check orientation, update if it has changed.
                    //
                    // Polling for changes is inefficient and wrong, but the
                    // useful stuff is hard to get at without a Dalvik VM.
                    err = SurfaceComposerClient::getDisplayInfo(mainDpy,
                            &mainDpyInfo);
                    if (err != NO_ERROR) {
                        ALOGW("getDisplayInfo(main) failed: %d", err);
                    } else if (orientation != mainDpyInfo.orientation) {
                        ALOGD("orientation changed, now %d", mainDpyInfo.orientation);
                        SurfaceComposerClient::Transaction t;
                        setDisplayProjection(t, virtualDpy, mainDpyInfo);
                        t.apply();
                        orientation = mainDpyInfo.orientation;
                    }
                }

                // If the virtual display isn't providing us with timestamps,
                // use the current time.  This isn't great -- we could get
                // decoded data in clusters -- but we're not expecting
                // to hit this anyway.
                if (ptsUsec == 0) {
                    ptsUsec = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
                }

                if (muxer == NULL) {
                    fwrite(buffers[bufIndex]->data(), 1, size, rawFp);
                    // Flush the data immediately in case we're streaming.
                    // We don't want to do this if all we've written is
                    // the SPS/PPS data because mplayer gets confused.
                    if ((flags & MediaCodec::BUFFER_FLAG_CODECCONFIG) == 0) {
                        fflush(rawFp);
                    }
                } else {
                    // The MediaMuxer docs are unclear, but it appears that we
                    // need to pass either the full set of BufferInfo flags, or
                    // (flags & BUFFER_FLAG_SYNCFRAME).
                    //
                    // If this blocks for too long we could drop frames.  We may
                    // want to queue these up and do them on a different thread.
                    ATRACE_NAME("write sample");
                    assert(trackIdx != -1);
                    // TODO
                    sp<ABuffer> buffer = new ABuffer(
                            buffers[bufIndex]->data(), buffers[bufIndex]->size());
                    err = muxer->writeSampleData(buffer, trackIdx,
                            ptsUsec, flags);
                    if (err != NO_ERROR) {
                        fprintf(stderr,
                            "Failed writing data to muxer (err=%d)\n", err);
                        return err;
                    }
                }
                debugNumFrames++;
            }
            err = encoder->releaseOutputBuffer(bufIndex);
            if (err != NO_ERROR) {
                fprintf(stderr, "Unable to release output buffer (err=%d)\n",
                        err);
                return err;
            }
            if ((flags & MediaCodec::BUFFER_FLAG_EOS) != 0) {
                // Not expecting EOS from SurfaceFlinger.  Go with it.
                ALOGI("Received end-of-stream");
                gStopRequested = true;
            }
            break;
        case -EAGAIN:                       // INFO_TRY_AGAIN_LATER
            ALOGV("Got -EAGAIN, looping");
            break;
        case android::INFO_FORMAT_CHANGED:    // INFO_OUTPUT_FORMAT_CHANGED
            {
                // Format includes CSD, which we must provide to muxer.
                ALOGV("Encoder format changed");
                sp<AMessage> newFormat;
                encoder->getOutputFormat(&newFormat);
                if (muxer != NULL) {
                    trackIdx = muxer->addTrack(newFormat);
                    ALOGV("Starting muxer");
                    err = muxer->start();
                    if (err != NO_ERROR) {
                        fprintf(stderr, "Unable to start muxer (err=%d)\n", err);
                        return err;
                    }
                }
            }
            break;
        case android::INFO_OUTPUT_BUFFERS_CHANGED:   // INFO_OUTPUT_BUFFERS_CHANGED
            // Not expected for an encoder; handle it anyway.
            ALOGV("Encoder buffers changed");
            err = encoder->getOutputBuffers(&buffers);
            if (err != NO_ERROR) {
                fprintf(stderr,
                        "Unable to get new output buffers (err=%d)\n", err);
                return err;
            }
            break;
        case INVALID_OPERATION:
            ALOGW("dequeueOutputBuffer returned INVALID_OPERATION");
            return err;
        default:
            fprintf(stderr,
                    "Got weird result %d from dequeueOutputBuffer\n", err);
            return err;
        }
    }

    ALOGV("Encoder stopping (req=%d)", gStopRequested);
    if (gVerbose) {
        printf("Encoder stopping; recorded %u frames in %" PRId64 " seconds\n",
                debugNumFrames, nanoseconds_to_seconds(
                        systemTime(CLOCK_MONOTONIC) - startWhenNsec));
        fflush(stdout);
    }
    return NO_ERROR;
}

/*
 * Raw H.264 byte stream output requested.  Send the output to stdout
 * if desired.  If the output is a tty, reconfigure it to avoid the
 * CRLF line termination that we see with "adb shell" commands.
 */
static FILE* prepareRawOutput(const char* fileName) {
    FILE* rawFp = NULL;

    if (strcmp(fileName, "-") == 0) {
        if (gVerbose) {
            fprintf(stderr, "ERROR: verbose output and '-' not compatible");
            return NULL;
        }
        rawFp = stdout;
    } else {
        rawFp = fopen(fileName, "w");
        if (rawFp == NULL) {
            fprintf(stderr, "fopen raw failed: %s\n", strerror(errno));
            return NULL;
        }
    }

    int fd = fileno(rawFp);
    if (isatty(fd)) {
        // best effort -- reconfigure tty for "raw"
        ALOGD("raw video output to tty (fd=%d)", fd);
        struct termios term;
        if (tcgetattr(fd, &term) == 0) {
            cfmakeraw(&term);
            if (tcsetattr(fd, TCSANOW, &term) == 0) {
                ALOGD("tty successfully configured for raw");
            }
        }
    }

    return rawFp;
}

static inline uint32_t floorToEven(uint32_t num) {
    return num & ~1;
}

/*
 * Main "do work" start point.
 *
 * Configures codec, muxer, and virtual display, then starts moving bits
 * around.
 */
static status_t recordScreen(const char* fileName) {
    status_t err;

    // Configure signal handler.
    err = configureSignals();
    if (err != NO_ERROR) return err;

    // Start Binder thread pool.  MediaCodec needs to be able to receive
    // messages from mediaserver.
    sp<ProcessState> self = ProcessState::self();
    self->startThreadPool();

    // Get main display parameters.
    const sp<IBinder> mainDpy = SurfaceComposerClient::getInternalDisplayToken();
    if (mainDpy == nullptr) {
        fprintf(stderr, "ERROR: no display\n");
        return NAME_NOT_FOUND;
    }

    DisplayInfo mainDpyInfo;
    err = SurfaceComposerClient::getDisplayInfo(mainDpy, &mainDpyInfo);
    if (err != NO_ERROR) {
        fprintf(stderr, "ERROR: unable to get display characteristics\n");
        return err;
    }

    if (gVerbose) {
        printf("Main display is %dx%d @%.2ffps (orientation=%u)\n",
                mainDpyInfo.viewportW, mainDpyInfo.viewportH, mainDpyInfo.fps,
                mainDpyInfo.orientation);
        fflush(stdout);
    }

    // Encoder can't take odd number as config
    if (gVideoWidth == 0) {
        gVideoWidth = floorToEven(mainDpyInfo.viewportW);
    }
    if (gVideoHeight == 0) {
        gVideoHeight = floorToEven(mainDpyInfo.viewportH);
    }

    // Configure and start the encoder.
    sp<MediaCodec> encoder;
    sp<FrameOutput> frameOutput;
    sp<IGraphicBufferProducer> encoderInputSurface;
    if (gOutputFormat != FORMAT_FRAMES && gOutputFormat != FORMAT_RAW_FRAMES) {
        err = prepareEncoder(mainDpyInfo.fps, &encoder, &encoderInputSurface);

        if (err != NO_ERROR && !gSizeSpecified) {
            // fallback is defined for landscape; swap if we're in portrait
            bool needSwap = gVideoWidth < gVideoHeight;
            uint32_t newWidth = needSwap ? kFallbackHeight : kFallbackWidth;
            uint32_t newHeight = needSwap ? kFallbackWidth : kFallbackHeight;
            if (gVideoWidth != newWidth && gVideoHeight != newHeight) {
                ALOGV("Retrying with 720p");
                fprintf(stderr, "WARNING: failed at %dx%d, retrying at %dx%d\n",
                        gVideoWidth, gVideoHeight, newWidth, newHeight);
                gVideoWidth = newWidth;
                gVideoHeight = newHeight;
                err = prepareEncoder(mainDpyInfo.fps, &encoder,
                        &encoderInputSurface);
            }
        }
        if (err != NO_ERROR) return err;

        // From here on, we must explicitly release() the encoder before it goes
        // out of scope, or we will get an assertion failure from stagefright
        // later on in a different thread.
    } else {
        // We're not using an encoder at all.  The "encoder input surface" we hand to
        // SurfaceFlinger will just feed directly to us.
        frameOutput = new FrameOutput();
        err = frameOutput->createInputSurface(gVideoWidth, gVideoHeight, &encoderInputSurface);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // Draw the "info" page by rendering a frame with GLES and sending
    // it directly to the encoder.
    // TODO: consider displaying this as a regular layer to avoid b/11697754
    if (gWantInfoScreen) {
        Overlay::drawInfoPage(encoderInputSurface);
    }

    // Configure optional overlay.
    sp<IGraphicBufferProducer> bufferProducer;
    sp<Overlay> overlay;
    if (gWantFrameTime) {
        // Send virtual display frames to an external texture.
        overlay = new Overlay(gMonotonicTime);
        err = overlay->start(encoderInputSurface, &bufferProducer);
        if (err != NO_ERROR) {
            if (encoder != NULL) encoder->release();
            return err;
        }
        if (gVerbose) {
            printf("Bugreport overlay created\n");
            fflush(stdout);
        }
    } else {
        // Use the encoder's input surface as the virtual display surface.
        bufferProducer = encoderInputSurface;
    }

    // Configure virtual display.
    sp<IBinder> dpy;
    err = prepareVirtualDisplay(mainDpyInfo, bufferProducer, &dpy);
    if (err != NO_ERROR) {
        if (encoder != NULL) encoder->release();
        return err;
    }

    sp<MediaMuxer> muxer = NULL;
    FILE* rawFp = NULL;
    switch (gOutputFormat) {
        case FORMAT_MP4:
        case FORMAT_WEBM:
        case FORMAT_3GPP: {
            // Configure muxer.  We have to wait for the CSD blob from the encoder
            // before we can start it.
            err = unlink(fileName);
            if (err != 0 && errno != ENOENT) {
                fprintf(stderr, "ERROR: couldn't remove existing file\n");
                abort();
            }
            int fd = open(fileName, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                fprintf(stderr, "ERROR: couldn't open file\n");
                abort();
            }
            if (gOutputFormat == FORMAT_MP4) {
                muxer = new MediaMuxer(fd, MediaMuxer::OUTPUT_FORMAT_MPEG_4);
            } else if (gOutputFormat == FORMAT_WEBM) {
                muxer = new MediaMuxer(fd, MediaMuxer::OUTPUT_FORMAT_WEBM);
            } else {
                muxer = new MediaMuxer(fd, MediaMuxer::OUTPUT_FORMAT_THREE_GPP);
            }
            close(fd);
            if (gRotate) {
                muxer->setOrientationHint(90);  // TODO: does this do anything?
            }
            break;
        }
        case FORMAT_H264:
        case FORMAT_FRAMES:
        case FORMAT_RAW_FRAMES: {
            rawFp = prepareRawOutput(fileName);
            if (rawFp == NULL) {
                if (encoder != NULL) encoder->release();
                return -1;
            }
            break;
        }
        default:
            fprintf(stderr, "ERROR: unknown format %d\n", gOutputFormat);
            abort();
    }

    if (gOutputFormat == FORMAT_FRAMES || gOutputFormat == FORMAT_RAW_FRAMES) {
        // TODO: if we want to make this a proper feature, we should output
        //       an outer header with version info.  Right now we never change
        //       the frame size or format, so we could conceivably just send
        //       the current frame header once and then follow it with an
        //       unbroken stream of data.

        // Make the EGL context current again.  This gets unhooked if we're
        // using "--bugreport" mode.
        // TODO: figure out if we can eliminate this
        frameOutput->prepareToCopy();

        while (!gStopRequested) {
            // Poll for frames, the same way we do for MediaCodec.  We do
            // all of the work on the main thread.
            //
            // Ideally we'd sleep indefinitely and wake when the
            // stop was requested, but this will do for now.  (It almost
            // works because wait() wakes when a signal hits, but we
            // need to handle the edge cases.)
            bool rawFrames = gOutputFormat == FORMAT_RAW_FRAMES;
            err = frameOutput->copyFrame(rawFp, 250000, rawFrames);
            if (err == ETIMEDOUT) {
                err = NO_ERROR;
            } else if (err != NO_ERROR) {
                ALOGE("Got error %d from copyFrame()", err);
                break;
            }
        }
    } else {
        // Main encoder loop.
        err = runEncoder(encoder, muxer, rawFp, mainDpy, dpy,
                mainDpyInfo.orientation);
        if (err != NO_ERROR) {
            fprintf(stderr, "Encoder failed (err=%d)\n", err);
            // fall through to cleanup
        }

        if (gVerbose) {
            printf("Stopping encoder and muxer\n");
            fflush(stdout);
        }
    }

    // Shut everything down, starting with the producer side.
    encoderInputSurface = NULL;
    SurfaceComposerClient::destroyDisplay(dpy);
    if (overlay != NULL) overlay->stop();
    if (encoder != NULL) encoder->stop();
    if (muxer != NULL) {
        // If we don't stop muxer explicitly, i.e. let the destructor run,
        // it may hang (b/11050628).
        err = muxer->stop();
    } else if (rawFp != stdout) {
        fclose(rawFp);
    }
    if (encoder != NULL) encoder->release();

    return err;
}

/*
 * Sends a broadcast to the media scanner to tell it about the new video.
 *
 * This is optional, but nice to have.
 */
static status_t notifyMediaScanner(const char* fileName) {
    // need to do allocations before the fork()
    String8 fileUrl("file://");
    fileUrl.append(fileName);

    const char* kCommand = "/system/bin/am";
    const char* const argv[] = {
            kCommand,
            "broadcast",
            "-a",
            "android.intent.action.MEDIA_SCANNER_SCAN_FILE",
            "-d",
            fileUrl.string(),
            NULL
    };
    if (gVerbose) {
        printf("Executing:");
        for (int i = 0; argv[i] != NULL; i++) {
            printf(" %s", argv[i]);
        }
        putchar('\n');
        fflush(stdout);
    }

    pid_t pid = fork();
    if (pid < 0) {
        int err = errno;
        ALOGW("fork() failed: %s", strerror(err));
        return -err;
    } else if (pid > 0) {
        // parent; wait for the child, mostly to make the verbose-mode output
        // look right, but also to check for and log failures
        int status;
        pid_t actualPid = TEMP_FAILURE_RETRY(waitpid(pid, &status, 0));
        if (actualPid != pid) {
            ALOGW("waitpid(%d) returned %d (errno=%d)", pid, actualPid, errno);
        } else if (status != 0) {
            ALOGW("'am broadcast' exited with status=%d", status);
        } else {
            ALOGV("'am broadcast' exited successfully");
        }
    } else {
        if (!gVerbose) {
            // non-verbose, suppress 'am' output
            ALOGV("closing stdout/stderr in child");
            int fd = open("/dev/null", O_WRONLY);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
        execv(kCommand, const_cast<char* const*>(argv));
        ALOGE("execv(%s) failed: %s\n", kCommand, strerror(errno));
        exit(1);
    }
    return NO_ERROR;
}

/*
 * Parses a string of the form "1280x720".
 *
 * Returns true on success.
 */
static bool parseWidthHeight(const char* widthHeight, uint32_t* pWidth,
        uint32_t* pHeight) {
    long width, height;
    char* end;

    // Must specify base 10, or "0x0" gets parsed differently.
    width = strtol(widthHeight, &end, 10);
    if (end == widthHeight || *end != 'x' || *(end+1) == '\0') {
        // invalid chars in width, or missing 'x', or missing height
        return false;
    }
    height = strtol(end + 1, &end, 10);
    if (*end != '\0') {
        // invalid chars in height
        return false;
    }

    *pWidth = width;
    *pHeight = height;
    return true;
}

/*
 * Accepts a string with a bare number ("4000000") or with a single-character
 * unit ("4m").
 *
 * Returns an error if parsing fails.
 */
static status_t parseValueWithUnit(const char* str, uint32_t* pValue) {
    long value;
    char* endptr;

    value = strtol(str, &endptr, 10);
    if (*endptr == '\0') {
        // bare number
        *pValue = value;
        return NO_ERROR;
    } else if (toupper(*endptr) == 'M' && *(endptr+1) == '\0') {
        *pValue = value * 1000000;  // check for overflow?
        return NO_ERROR;
    } else {
        fprintf(stderr, "Unrecognized value: %s\n", str);
        return UNKNOWN_ERROR;
    }
}

/*
 * Dumps usage on stderr.
 */
static void usage() {
    fprintf(stderr,
        "Usage: screenrecord [options] <filename>\n"
        "\n"
        "Android screenrecord v%d.%d.  Records the device's display to a .mp4 file.\n"
        "\n"
        "Options:\n"
        "--size WIDTHxHEIGHT\n"
        "    Set the video size, e.g. \"1280x720\".  Default is the device's main\n"
        "    display resolution (if supported), 1280x720 if not.  For best results,\n"
        "    use a size supported by the AVC encoder.\n"
        "--bit-rate RATE\n"
        "    Set the video bit rate, in bits per second.  Value may be specified as\n"
        "    bits or megabits, e.g. '4000000' is equivalent to '4M'.  Default %dMbps.\n"
        "--bugreport\n"
        "    Add additional information, such as a timestamp overlay, that is helpful\n"
        "    in videos captured to illustrate bugs.\n"
        "--time-limit TIME\n"
        "    Set the maximum recording time, in seconds.  Default / maximum is %d.\n"
        "--verbose\n"
        "    Display interesting information on stdout.\n"
        "--help\n"
        "    Show this message.\n"
        "\n"
        "Recording continues until Ctrl-C is hit or the time limit is reached.\n"
        "\n",
        kVersionMajor, kVersionMinor, gBitRate / 1000000, gTimeLimitSec
        );
}

/*
 * Parses args and kicks things off.
 */
int main(int argc, char* const argv[]) {
    static const struct option longOptions[] = {
        { "help",               no_argument,        NULL, 'h' },
        { "verbose",            no_argument,        NULL, 'v' },
        { "size",               required_argument,  NULL, 's' },
        { "bit-rate",           required_argument,  NULL, 'b' },
        { "time-limit",         required_argument,  NULL, 't' },
        { "bugreport",          no_argument,        NULL, 'u' },
        // "unofficial" options
        { "show-device-info",   no_argument,        NULL, 'i' },
        { "show-frame-time",    no_argument,        NULL, 'f' },
        { "rotate",             no_argument,        NULL, 'r' },
        { "output-format",      required_argument,  NULL, 'o' },
        { "codec-name",         required_argument,  NULL, 'N' },
        { "monotonic-time",     no_argument,        NULL, 'm' },
        { "persistent-surface", no_argument,        NULL, 'p' },
        { "bframes",            required_argument,  NULL, 'B' },
        { NULL,                 0,                  NULL, 0 }
    };

    while (true) {
        int optionIndex = 0;
        int ic = getopt_long(argc, argv, "", longOptions, &optionIndex);
        if (ic == -1) {
            break;
        }

        switch (ic) {
        case 'h':
            usage();
            return 0;
        case 'v':
            gVerbose = true;
            break;
        case 's':
            if (!parseWidthHeight(optarg, &gVideoWidth, &gVideoHeight)) {
                fprintf(stderr, "Invalid size '%s', must be width x height\n",
                        optarg);
                return 2;
            }
            if (gVideoWidth == 0 || gVideoHeight == 0) {
                fprintf(stderr,
                    "Invalid size %ux%u, width and height may not be zero\n",
                    gVideoWidth, gVideoHeight);
                return 2;
            }
            gSizeSpecified = true;
            break;
        case 'b':
            if (parseValueWithUnit(optarg, &gBitRate) != NO_ERROR) {
                return 2;
            }
            if (gBitRate < kMinBitRate || gBitRate > kMaxBitRate) {
                fprintf(stderr,
                        "Bit rate %dbps outside acceptable range [%d,%d]\n",
                        gBitRate, kMinBitRate, kMaxBitRate);
                return 2;
            }
            break;
        case 't':
            gTimeLimitSec = atoi(optarg);
            if (gTimeLimitSec == 0 || gTimeLimitSec > kMaxTimeLimitSec) {
                fprintf(stderr,
                        "Time limit %ds outside acceptable range [1,%d]\n",
                        gTimeLimitSec, kMaxTimeLimitSec);
                return 2;
            }
            break;
        case 'u':
            gWantInfoScreen = true;
            gWantFrameTime = true;
            break;
        case 'i':
            gWantInfoScreen = true;
            break;
        case 'f':
            gWantFrameTime = true;
            break;
        case 'r':
            // experimental feature
            gRotate = true;
            break;
        case 'o':
            if (strcmp(optarg, "mp4") == 0) {
                gOutputFormat = FORMAT_MP4;
            } else if (strcmp(optarg, "h264") == 0) {
                gOutputFormat = FORMAT_H264;
            } else if (strcmp(optarg, "webm") == 0) {
                gOutputFormat = FORMAT_WEBM;
            } else if (strcmp(optarg, "3gpp") == 0) {
                gOutputFormat = FORMAT_3GPP;
            } else if (strcmp(optarg, "frames") == 0) {
                gOutputFormat = FORMAT_FRAMES;
            } else if (strcmp(optarg, "raw-frames") == 0) {
                gOutputFormat = FORMAT_RAW_FRAMES;
            } else {
                fprintf(stderr, "Unknown format '%s'\n", optarg);
                return 2;
            }
            break;
        case 'N':
            gCodecName = optarg;
            break;
        case 'm':
            gMonotonicTime = true;
            break;
        case 'p':
            gPersistentSurface = true;
            break;
        case 'B':
            if (parseValueWithUnit(optarg, &gBframes) != NO_ERROR) {
                return 2;
            }
            break;
        default:
            if (ic != '?') {
                fprintf(stderr, "getopt_long returned unexpected value 0x%x\n", ic);
            }
            return 2;
        }
    }

    if (optind != argc - 1) {
        fprintf(stderr, "Must specify output file (see --help).\n");
        return 2;
    }

    const char* fileName = argv[optind];
    if (gOutputFormat == FORMAT_MP4) {
        // MediaMuxer tries to create the file in the constructor, but we don't
        // learn about the failure until muxer.start(), which returns a generic
        // error code without logging anything.  We attempt to create the file
        // now for better diagnostics.
        int fd = open(fileName, O_CREAT | O_RDWR, 0644);
        if (fd < 0) {
            fprintf(stderr, "Unable to open '%s': %s\n", fileName, strerror(errno));
            return 1;
        }
        close(fd);
    }

    status_t err = recordScreen(fileName);
    if (err == NO_ERROR) {
        // Try to notify the media scanner.  Not fatal if this fails.
        notifyMediaScanner(fileName);
    }
    ALOGD(err == NO_ERROR ? "success" : "failed");
    return (int) err;
}
