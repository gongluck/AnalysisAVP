package com.gongluck.helper;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;

public class MediaCodecHelper {

    static final String TAG = "MediaCodecHelper";

    private MediaCodec mMediaCodec;
    private MediaFormat mMediaFormat;

    private String mMime;
    private int mWidth;
    private int mHeight;
    private int mProfile;
    private int mLevel;
    private int mMode;
    private int mBitrate;
    private int mFrameRate;
    private int mIFrameInterval;
    private int mColorFormat;

    private VideoEncodeCallback mVideoEncodeCallback;
    private long mStartTime;

    private boolean mEncoding = false;
    private int mQueueSize = 60;
    private ArrayBlockingQueue<byte[]> mInputQueue = new ArrayBlockingQueue<>(mQueueSize);
    private int mTimeOutus = 12000;
    private Thread mEncodeThread;

    //获取编解码器信息
    public boolean GetMediaCodecList(List<MediaCodecInfo> infos) {
        infos.clear();
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; ++i) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            infos.add(info);

            Log.d(TAG, info.getName() + "==========");
            for (String type : info.getSupportedTypes()) {
                Log.i(TAG, "\tsupport type : " + type + ", " + (info.isEncoder() ? "encoder" : "decoder"));
            }
        }

        return true;
    }

    //设置视频编码参数
    public boolean SetVideoEncodeParams(String mime, int width, int height, int profile, int level, int mode, int bitRate, int frameRate, int iFrameInterval, int colorFormat) {
        if (mMediaCodec != null) {
            Log.e(TAG, "have opened encoder.");
            return false;
        }

        mMime = mime;
        mWidth = width;
        mHeight = height;
        mProfile = profile;
        mLevel = level;
        mMode = mode;
        mBitrate = bitRate;
        mFrameRate = frameRate;
        mIFrameInterval = iFrameInterval;
        mColorFormat = colorFormat;

        return true;
    }

    //设置视频编码回调
    public interface VideoEncodeCallback {
        void EncodeCallback(MediaCodec.BufferInfo info, ByteBuffer buffer);
        void OutputFormatChange(MediaFormat format);
    }

    public boolean SetVideoEncodeCallback(VideoEncodeCallback cb) {
        if (mMediaCodec != null) {
            Log.e(TAG, "have opened encoder.");
            return false;
        }

        mVideoEncodeCallback = cb;

        return true;
    }

    //开始视频编码
    public boolean StartEncodeByType(String type) throws IOException {
        StopEncode();
        setParams();

        mMediaCodec = MediaCodec.createEncoderByType(type);

        return startEncode();
    }

    //开始视频编码
    public boolean StartEncodeByName(String name) throws IOException {
        StopEncode();
        setParams();

        mMediaCodec = MediaCodec.createByCodecName(name);

        return startEncode();
    }

    //停止编码
    public boolean StopEncode() {
        if (mMediaCodec == null) {
            Log.e(TAG, "have not opened encoder.");
            return false;
        }

        mEncoding = false;
        mMediaCodec.flush();

        if (mEncodeThread != null) {
            try {
                mEncodeThread.stop();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        mEncodeThread = null;
        mStartTime = 0;

        mMediaCodec.stop();
        mMediaCodec.release();
        mMediaCodec = null;

        return true;
    }

    //重置参数
    public boolean Reset() {
        mMediaFormat = null;

        mMime = null;
        mWidth = 0;
        mHeight = 0;
        mProfile = 0;
        mLevel = 0;
        mMode = 0;
        mBitrate = 0;
        mFrameRate = 0;
        mIFrameInterval = 0;
        mColorFormat = 0;

        return true;
    }

    //输入待编码数据
    public boolean InputBuffer(byte[] buffer) {
        if (mMediaCodec == null) {
            Log.e(TAG, "have not opened encoder.");
            return false;
        }

        while (mInputQueue.size() >= mQueueSize) {
            Log.w(TAG, "input queue full");
            mInputQueue.poll();
        }

        mInputQueue.add(buffer);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    private boolean setParams() {
        mMediaFormat = MediaFormat.createVideoFormat(mMime, mWidth, mHeight);
        mMediaFormat.setInteger(MediaFormat.KEY_PROFILE, mProfile);
        mMediaFormat.setInteger(MediaFormat.KEY_LEVEL, mLevel);
        mMediaFormat.setInteger(MediaFormat.KEY_BITRATE_MODE, mMode);
        mMediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, mBitrate);
        mMediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, mFrameRate); //帧率
        mMediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, mIFrameInterval); //framerate为单位的I帧间隔(GOP)
        mMediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, mColorFormat);

        return true;
    }

    private boolean startEncode() {
        mMediaCodec.configure(mMediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mMediaCodec.start();

        mEncoding = true;
        mEncodeThread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (mEncoding) {
                    if (mInputQueue.size() > 0) {
                        byte[] input = mInputQueue.poll();
                        int inputBufferIndex = mMediaCodec.dequeueInputBuffer(0);
                        if (inputBufferIndex >= 0) { //输入队列有可用缓冲区
                            ByteBuffer inputBuffer = mMediaCodec.getInputBuffers()[inputBufferIndex];
                            inputBuffer.clear();
                            inputBuffer.put(input);
                            mMediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, (System.nanoTime() - mStartTime) / 1000/*微秒*/, 0);
                        }
                        //取出编码数据
                        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
                        int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(bufferInfo, mTimeOutus);
                        switch (outputBufferIndex) {
                            case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                                MediaFormat format = mMediaCodec.getOutputFormat();
                                if(mVideoEncodeCallback != null) {
                                    mVideoEncodeCallback.OutputFormatChange(format);
                                }
                                break;
                            default: {
                                while (outputBufferIndex >= 0) { //输出队列有可用缓冲区
                                    if (mVideoEncodeCallback != null) {
                                        ByteBuffer outputBuffer = mMediaCodec.getOutputBuffers()[outputBufferIndex];
                                        mVideoEncodeCallback.EncodeCallback(bufferInfo, outputBuffer);
                                    }

                                    mMediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                                    outputBufferIndex = mMediaCodec.dequeueOutputBuffer(bufferInfo, mTimeOutus);
                                }
                            }
                            break;
                        }

                    } else {//队列为空
                        try {
                            Thread.sleep(10);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        });
        mStartTime = System.nanoTime();
        mEncodeThread.start();

        return true;
    }
}
