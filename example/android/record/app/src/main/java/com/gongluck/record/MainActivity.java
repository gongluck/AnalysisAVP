package com.gongluck.record;

import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {

    String TAG = "android-reocder";
    MediaProjectionManager mMediaProjectionManager;
    MediaProjection mMediaProjection;
    MediaCodec mEncoder;
    Surface mSurface;
    VirtualDisplay mVirtualDisplay;
    byte[] configbyte;
    BufferedOutputStream outputStream;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //recorder
        //getSystemService是Android很重要的一个API，它是Activity的一个方法，根据传入的NAME来取得对应的Object，然后转换成相应的服务对象。
        mMediaProjectionManager = (MediaProjectionManager) getSystemService(MEDIA_PROJECTION_SERVICE);
        Intent captureIntent = mMediaProjectionManager.createScreenCaptureIntent();
        startActivityForResult(captureIntent, 999);
    }

    @Override
    //重写onActivityResult()方法，获取授权结果
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(resultCode != Activity.RESULT_OK)
        {
            Log.e(TAG, "onActivityResult failed : " + String.valueOf(resultCode));
            return;
        }
        try {
            //创建MediaProjection 的实例
            mMediaProjection = mMediaProjectionManager.getMediaProjection(resultCode, data);
            //创建最小的视频格式
            MediaFormat format = MediaFormat.createVideoFormat("video/avc", 720, 1280);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            format.setInteger(MediaFormat.KEY_BIT_RATE, 200000);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, 15);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 15);
            //创建编码器
            mEncoder = MediaCodec.createEncoderByType("video/avc");
            mEncoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mSurface = mEncoder.createInputSurface();
            mEncoder.start();

            //创建一个VirtualDisplay的实例
            mVirtualDisplay = mMediaProjection.createVirtualDisplay("display", 720, 1280, 1,
                    DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC, mSurface, null, null);

            File file = new File("/data/save.h264");
            if (file.exists()) {
                file.delete();
            }
            else {
                file.createNewFile();
            }
            try {
                outputStream = new BufferedOutputStream(new FileOutputStream(file));
            } catch (Exception e) {
                e.printStackTrace();
            }

            int index = 0;
            MediaCodec.BufferInfo mBufferInfo = new MediaCodec.BufferInfo();
            while (++index < 1000) {
                //返回要用有效数据填充的输入缓冲区的索引（等待获取缓存区会阻塞）
                int outputBufferIndex = mEncoder.dequeueOutputBuffer(mBufferInfo, 16 * 1000);
                switch (outputBufferIndex) {
                    case MediaCodec.INFO_TRY_AGAIN_LATER:
                        break;
                    case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                    case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                        Log.i(TAG, "outputBufferIndex speicial : " + String.valueOf(outputBufferIndex));
                        break;
                }
                if (outputBufferIndex >= 0) {
                    //取出队列数据
                    ByteBuffer outputBuffer = mEncoder.getOutputBuffers()[outputBufferIndex];
                    byte[] outData = new byte[mBufferInfo.size];
                    outputBuffer.get(outData);
                    Log.i(TAG, "flags : " + mBufferInfo.flags);
                    switch (mBufferInfo.flags) {
                        case 0:
                        case android.media.MediaCodec.BUFFER_FLAG_END_OF_STREAM:
                            Log.i(TAG, "frame size : " + outData.length);
                            outputStream.write(outData, 0, outData.length);
                            break;
                        case android.media.MediaCodec.BUFFER_FLAG_KEY_FRAME:
                            byte[] keyframe = new byte[mBufferInfo.size + configbyte.length];
                            System.arraycopy(configbyte, 0, keyframe, 0, configbyte.length);
                            System.arraycopy(outData, 0, keyframe, configbyte.length, outData.length);
                            Log.i(TAG, "keyframe size : " + String.valueOf(outData.length));
                            outputStream.write(keyframe, 0, keyframe.length);
                            break;
                        case android.media.MediaCodec.BUFFER_FLAG_CODEC_CONFIG:
                            configbyte = new byte[mBufferInfo.size];
                            configbyte = outData;
                            break;
                        default:
                            break;
                    }

                    mEncoder.releaseOutputBuffer(outputBufferIndex, false);
                }
            }
            //结束
            if (mEncoder != null) {
                mEncoder.stop();
                mEncoder.release();
                mEncoder = null;
            }
            if (mVirtualDisplay != null) {
                mVirtualDisplay.release();
                mVirtualDisplay = null;
            }
        } catch (IOException e) {
            Log.e(TAG, e.toString());
        }
    }

}
