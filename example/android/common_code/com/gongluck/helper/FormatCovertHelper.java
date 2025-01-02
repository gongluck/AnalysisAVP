package com.gongluck.helper;

import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;

public class FormatCovertHelper {

    static final String TAG = "FormatCovertHelper";

    public boolean NV21ToNV12(byte[] nv21, int width, int height) {
        return swapUV(nv21, width, height);
    }

    public boolean NV12ToNV21(byte[] nv12, int width, int height) {
        return swapUV(nv12, width, height);
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

    public boolean CutStride(byte[] src, int width, int height, int pixel, int stride, byte[] dst) {
        for (int i = 0; i < height; i++) {
            System.arraycopy(src, i * stride * pixel, dst, i * width * pixel, width * pixel);
        }
        return true;
    }

    public void SaveFile(byte[] data, String path) {
        try {
            BufferedOutputStream outputStream = new BufferedOutputStream(new FileOutputStream(new File(path)));
            outputStream.write(data, 0, data.length);
            outputStream.flush();
            outputStream.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    private boolean swapUV(byte[] frame, int width, int height) {
        int framesize = width * height;
        for (int i = 0; i < framesize / 2; ) {
            byte tmp = frame[framesize + i];
            frame[framesize + i] = frame[framesize + i + 1];
            frame[framesize + i + 1] = tmp;
            i += 2;
        }
        return true;
    }
}
