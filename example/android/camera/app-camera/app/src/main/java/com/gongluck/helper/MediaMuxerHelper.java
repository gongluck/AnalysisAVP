package com.gongluck.helper;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.IOException;
import java.nio.ByteBuffer;

@RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
public class MediaMuxerHelper {

    static final String TAG = "MediaMuxerHelper";

    private MediaMuxer mMediaMuxer;

    //创建封装
    public boolean OpenMux(String path, int format) throws IOException {
        StopMux();

        mMediaMuxer = new MediaMuxer(path, format);

        return true;
    }

    //添加轨
    public int AddTrack(MediaFormat format) {
        if (mMediaMuxer == null) {
            Log.e(TAG, "have not open mux.");
            return -1;
        }

        return mMediaMuxer.addTrack(format);
    }

    //开始封装
    public boolean StartMux() {
        if (mMediaMuxer == null) {
            Log.e(TAG, "have not open mux.");
            return false;
        }

        mMediaMuxer.start();

        return true;
    }

    //写入数据
    public boolean WriteSample(int trackIndex, ByteBuffer buffer, MediaCodec.BufferInfo info) {
        if (mMediaMuxer == null) {
            Log.e(TAG, "have not open mux.");
            return false;
        }

        mMediaMuxer.writeSampleData(trackIndex, buffer, info);

        return true;
    }

    //停止封装
    public boolean StopMux() {
        if (mMediaMuxer == null) {
            Log.e(TAG, "have not open mux.");
            return false;
        }

        mMediaMuxer.stop();
        mMediaMuxer.release();
        mMediaMuxer = null;

        return true;
    }

}
