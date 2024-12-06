package com.gongluck.helper;

import android.util.Log;

public class RotationHelper {

    static final String TAG = "RotationHelper";

    //根据系统角度计算旋转角度
    //systemOrientation 逆时针
    //返回 顺时针
    public int CaluateRotation(boolean facing, int cameraRotation, int systemOrientation) {
        //https://blog.csdn.net/qq_18757521/article/details/99711438
        int cameraOrientation = cameraRotation;
        Log.d(TAG, "system: " + systemOrientation + ", camera: " + cameraOrientation + ", facing: " + facing);
        if (facing) {
            return (cameraOrientation + 90 * systemOrientation) % 360;
        } else {
            return (180 + cameraOrientation - 90 * systemOrientation + 360) % 360;
        }
    }

    public byte[] NV21ToNV12(byte[] nv21, int width, int height) {
        byte[] yuv = new byte[width * height * 3 / 2];
        int framesize = width * height;
        int i = 0, j = 0;
        for (i = 0; i < framesize; i++) {
            yuv[i] = nv21[i];
        }
        for (j = 0; j < framesize / 2; j += 2) {
            yuv[framesize + j - 1] = nv21[j + framesize];
        }
        for (j = 0; j < framesize / 2; j += 2) {
            yuv[framesize + j] = nv21[j + framesize - 1];
        }
        return yuv;
    }

    public byte[] Rotation(int rotation, byte[] src, int width, int height) {
        switch(rotation % 360) {
            case 90:
                return Rotate90(src, width, height);
            case 180:
                return Rotate180(src, width, height);
            case 270:
                return Rotate270(src, width, height);
            default:
                return src;
        }
    }

    public byte[] Rotate90(byte[] src, int width, int height) {
        byte[] yuv = new byte[width * height * 3 / 2];
        int i = 0;
        for (int x = 0; x < width; x++) {
            for (int y = height - 1; y >= 0; y--) {
                yuv[i] = src[y * width + x];
                i++;
            }
        }
        i = width * height * 3 / 2 - 1;
        for (int x = width - 1; x > 0; x = x - 2) {
            for (int y = 0; y < height / 2; y++) {
                yuv[i] = src[(width * height) + (y * width) + x];
                i--;
                yuv[i] = src[(width * height) + (y * width) + (x - 1)];
                i--;
            }
        }
        return yuv;
    }

    public byte[] Rotate180(byte[] src, int width, int height) {
        byte[] yuv = new byte[width * height * 3 / 2];
        int i = 0;
        int count = 0;
        for (i = width * height - 1; i >= 0; i--) {
            yuv[count] = src[i];
            count++;
        }
        i = width * height * 3 / 2 - 1;
        for (i = width * height * 3 / 2 - 1; i >= width * height; i -= 2) {
            yuv[count++] = src[i - 1];
            yuv[count++] = src[i];
        }
        return yuv;
    }

    public byte[] Rotate270(byte[] src, int width, int height) {
        byte[] yuv = new byte[width * height * 3 / 2];
        int nWidth = 0, nHeight = 0;
        int wh = 0;
        int uvHeight = 0;
        if (width != nWidth || height != nHeight) {
            nWidth = width;
            nHeight = height;
            wh = width * height;
            uvHeight = height >> 1; // uvHeight = height / 2
        }

        int k = 0;
        for (int i = 0; i < width; i++) {
            int nPos = 0;
            for (int j = 0; j < height; j++) {
                yuv[k] = src[nPos + i];
                k++;
                nPos += width;
            }
        }
        for (int i = 0; i < width; i += 2) {
            int nPos = wh;
            for (int j = 0; j < uvHeight; j++) {
                yuv[k] = src[nPos + i];
                yuv[k + 1] = src[nPos + i + 1];
                k += 2;
                nPos += width;
            }
        }
        return Rotate180(Rotate90(src, width, height), width, height);
    }

    //翻转
    public void Mirror(byte[] src, int w, int h) { //src是原始yuv数组
        int i;
        int index;
        byte temp;
        int a, b;
        //mirror y
        for (i = 0; i < h; i++) {
            a = i * w;
            b = (i + 1) * w - 1;
            while (a < b) {
                temp = src[a];
                src[a] = src[b];
                src[b] = temp;
                a++;
                b--;
            }
        }

        // mirror u and v
        index = w * h;
        for (i = 0; i < h / 2; i++) {
            a = i * w;
            b = (i + 1) * w - 2;
            while (a < b) {
                temp = src[a + index];
                src[a + index] = src[b + index];
                src[b + index] = temp;

                temp = src[a + index + 1];
                src[a + index + 1] = src[b + index + 1];
                src[b + index + 1] = temp;
                a += 2;
                b -= 2;
            }
        }
    }
}
