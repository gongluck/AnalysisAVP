package com.gongluck.helper;

import android.util.Log;

public class FormatCovertHelper {

    static final String TAG = "FormatCovertHelper";

    public boolean NV21ToNV12(byte[] nv21, int width, int height) {
        int framesize = width * height;
        Log.d(TAG, "framesize: " + framesize);
        for (int i = 0; i < framesize / 2; ) {
            byte tmp = nv21[framesize + i];
            nv21[framesize + i] = nv21[framesize + i + 1];
            nv21[framesize + i + 1] = tmp;
            i += 2;
        }
        return true;
    }

    public boolean YUV420ToNV21(byte[] yuv420, int width, int height) {
        int frameSize = width * height;
        int uvSize = frameSize / 2;
        byte[] uvData = new byte[uvSize];
        System.arraycopy(yuv420, frameSize, uvData, 0, uvSize);
        for (int i = 0; i < uvSize / 2; i++) {
            yuv420[frameSize + 2 * i] = uvData[uvSize / 2 + i];
            yuv420[frameSize + 2 * i + 1] = uvData[i];
        }
        return true;
    }
}
