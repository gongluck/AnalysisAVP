//参考：
//https://www.jianshu.com/p/9a51270b69ea
//https://blog.csdn.net/import_sadaharu/article/details/52744899?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_title~default-0.no_search_link&spm=1001.2101.3001.4242
//https://blog.csdn.net/ss182172633/article/details/50256733
package com.gongluck.camera;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.opengl.GLES11Ext;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;

public class MainActivity extends AppCompatActivity {
    static final String TAG = "Camera";
    private SurfaceView mSurfaceView = null;
    private ImageView mImageView = null;
    private Button mStart = null;
    private Button mStop = null;

    private int mWidth = 1280;
    private int mHeight = 720;

    //rotation
    int mRotation = 0;
    boolean usefacing = true;//是否使用前置摄像头

    //Camera
    private Camera mCamera = null;
    private SurfaceTexture mSurfaceTexture = new SurfaceTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
    private byte[] mPreviewData = new byte[mWidth * mHeight * 3 / 2];
    private boolean mUseSurfacePreview = true;

    //MediaCodec
    private MediaCodec mMediaCodec = null;
    private BufferedOutputStream mBufferedOutputStream = null;
    private boolean mEncoding = false;
    private int mQueueSize = 10;
    private ArrayBlockingQueue<byte[]> mYUVQueue = new ArrayBlockingQueue<byte[]>(mQueueSize);
    private byte[] mSpsPps = null;
    private int mSleepTime = 12000;
    private Thread mWork = null;

    //帧回调
    Camera.PreviewCallback mPrewviewCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            //设置下一帧回调，调试一天，原来是逻辑错误没有调用addCallbackBuffer导致编码出的264数据不能正常播放，只有一帧有效数据
            //放在最前面，以防忘记！
            mCamera.addCallbackBuffer(mPreviewData);

            Camera.Parameters parameters = mCamera.getParameters();
            //Log.i(TAG, "parameters : " + parameters.flatten());
            int imageFormat = parameters.getPreviewFormat();
            int w = parameters.getPreviewSize().width;
            int h = parameters.getPreviewSize().height;
            Log.i(TAG, "data info:\nformat = " + imageFormat + "\nwidth = " + w + "\nheight = " + h);

            try {
                //保存yuv
                File file = new File(Environment.getExternalStorageDirectory().getPath() + "/save.yuv");
                if (file.exists()) {
                    file.delete();
                } else {
                    file.createNewFile();
                }
                BufferedOutputStream outputStream = new BufferedOutputStream(new FileOutputStream(file));
                outputStream.write(data, 0, data.length);

                //保存jpeg
                String path = Environment.getExternalStorageDirectory().getPath() + "/save.jpg";
                Rect rect = new Rect(0, 0, w, h);
                YuvImage yuvImg = new YuvImage(data, imageFormat, w, h, null);
                BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(path));
                yuvImg.compressToJpeg(rect, 100, bos);
                bos.flush();
                bos.close();

                if (!mUseSurfacePreview) {
                    mSurfaceView.setVisibility(View.INVISIBLE);

                    //显示到imageview
                    ByteArrayOutputStream stream = new ByteArrayOutputStream();
                    yuvImg.compressToJpeg(new Rect(0, 0, w, h), 100, stream);
                    Bitmap bm = BitmapFactory.decodeByteArray(stream.toByteArray(), 0, stream.size());
                    stream.close();
                    mImageView.setImageBitmap(bm);
                    mImageView.setRotation(0);
                }

                //推送待编码数据
                if (mYUVQueue.size() >= mQueueSize) {
                    Log.e(TAG, "YUVQueue full");
                    mYUVQueue.poll();
                }
                //靠这里打日志发现没有调用addCallbackBuffer的问题
                //Log.i(TAG, "input size : " + data.length);
                mYUVQueue.add(data);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //控件
        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        mImageView = (ImageView) findViewById(R.id.imageView);
        mStart = (Button) findViewById(R.id.btn_start);
        mStop = (Button) findViewById(R.id.btn_stop);

        mStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onCameraOpen(v);
            }
        });

        mStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onCameraStop(v);
            }
        });
    }

    void onCameraOpen(View v) {
        onCameraStop(v);
        Log.i(TAG, "Start...");
        int nums = Camera.getNumberOfCameras();
        Log.i(TAG, "There has " + nums + " camera devices.");

        Camera.CameraInfo camerainfo = new Camera.CameraInfo();
        int device = -1;
        for (int i = 0; i < nums; ++i) {
            Camera.getCameraInfo(i, camerainfo);
            Log.i(TAG, "The " + i + " camera device info:\nfacing=" + camerainfo.facing);
            if (usefacing && camerainfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                device = i;
            } else if (!usefacing && camerainfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
                device = i;
            }
        }

        mCamera = Camera.open(device);
        Camera.Parameters cameraParams = mCamera.getParameters();
        List<Integer> formats = cameraParams.getSupportedPictureFormats();
        for (int i : formats) {
            Log.i(TAG, "Support picture format : " + i);
        }
        List<Camera.Size> sizes = cameraParams.getSupportedPictureSizes();
        for (Camera.Size s : sizes) {
            Log.i(TAG, "Support picture size : " + s.width + " x " + s.height);
        }

        formats = cameraParams.getSupportedPreviewFormats();
        for (int i : formats) {
            Log.i(TAG, "Support preview format : " + i);
        }
        sizes = cameraParams.getSupportedPreviewSizes();
        for (Camera.Size s : sizes) {
            Log.i(TAG, "Support preview size : " + s.width + " x " + s.height);
        }

        cameraParams.setPreviewFormat(ImageFormat.NV21);
        cameraParams.setPreviewSize(mWidth, mHeight);
        cameraParams.setPictureFormat(ImageFormat.NV21);
        cameraParams.setPictureSize(mWidth, mHeight);
        cameraParams.setZoom(0);
        cameraParams.setRotation(0);
        if(!usefacing)
        {
            cameraParams.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
        }
        mCamera.setParameters(cameraParams);

        mRotation = this.getWindowManager().getDefaultDisplay().getRotation();
        Log.i(TAG, "rotation : " + mRotation);
        mCamera.setDisplayOrientation(90 * ((5 - mRotation) % 4));//mUseSurfacePreview==true
        mCamera.startPreview();
        mCamera.setPreviewCallbackWithBuffer(mPrewviewCallback);
        mCamera.addCallbackBuffer(mPreviewData);
        try {
            if (mUseSurfacePreview) {
                mCamera.setPreviewDisplay(mSurfaceView.getHolder());
            } else {
                mCamera.setPreviewTexture(mSurfaceTexture);
            }

            String codecname = null;
            int counts = MediaCodecList.getCodecCount();
            Log.i(TAG, "Support codec counts : " + counts);
            for (int i = 0; i < counts; ++i) {
                MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
                Log.i(TAG, "Support codec : " + info.getName() +
                        ", isencoder : " + info.isEncoder() +
                        ", ishwaccel : " + info.isHardwareAccelerated() +
                        ", issoftware : " + info.isSoftwareOnly());
                for (String type : info.getSupportedTypes()) {
                    Log.i(TAG, "\tsupport type : " + type);
                    if (type.equals(MediaFormat.MIMETYPE_VIDEO_AVC) && info.isEncoder() && info.isHardwareAccelerated()) {
                        codecname = info.getName();
                        Log.i(TAG, "\tmaybe use : " + codecname);
                    }
                }
            }
            Log.i(TAG, "++++ use : " + codecname);

            //编码h264
            MediaFormat mediaFormat = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, mHeight, mWidth);
            mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
            mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, mWidth * mHeight * 1);
            mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
            mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);//framerate为单位

            //mMediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
            mMediaCodec = MediaCodec.createByCodecName(codecname);
            mMediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mMediaCodec.start();

            File file = new File(Environment.getExternalStorageDirectory().getPath() + "/save.h264");
            if (file.exists()) {
                file.delete();
            }
            mBufferedOutputStream = new BufferedOutputStream(new FileOutputStream(file));

            //编码线程
            mWork = new Thread(new Runnable() {
                @Override
                public void run() {
                    mEncoding = true;
                    byte[] input = null;
                    while (mEncoding) {
                        long pts = System.nanoTime();
                        if (mYUVQueue.size() > 0) {
                            input = mYUVQueue.poll();
                            //靠这里打日志发现没有调用addCallbackBuffer的问题
                            //Log.i(TAG, "output szie : " + input.length);
                            //demo只考虑竖屏
                            if (usefacing) {
                                input = NV21ToNV12(Rotate270(input, mWidth, mHeight), mHeight, mWidth);
                                Mirror(input, mHeight, mWidth);
                            } else {
                                input = NV21ToNV12(Rotate90(input, mWidth, mHeight), mHeight, mWidth);
                            }
                        }
                        if (input != null) {
                            try {
                                //输入待编码数据
                                int inputBufferIndex = mMediaCodec.dequeueInputBuffer(0);
                                //Log.i(TAG, "inputBufferIndex : " + inputBufferIndex);
                                if (inputBufferIndex >= 0) {
                                    ByteBuffer inputBuffer = mMediaCodec.getInputBuffers()[inputBufferIndex];
                                    inputBuffer.clear();
                                    inputBuffer.put(input);
                                    mMediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, pts, 0);
                                }
                                //取出编码数据
                                MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
                                int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(bufferInfo, mSleepTime);
                                //Log.i(TAG, "outputBufferIndex : " + outputBufferIndex);
                                while (outputBufferIndex >= 0) {
                                    ByteBuffer outputBuffer = mMediaCodec.getOutputBuffers()[outputBufferIndex];
                                    byte[] outData = new byte[bufferInfo.size];
                                    outputBuffer.get(outData);
                                    switch (bufferInfo.flags) {
                                        case MediaCodec.BUFFER_FLAG_CODEC_CONFIG:
                                            mSpsPps = new byte[bufferInfo.size];
                                            mSpsPps = outData;
                                            break;
                                        case MediaCodec.BUFFER_FLAG_KEY_FRAME:
                                            byte[] keyframe = new byte[bufferInfo.size + mSpsPps.length];
                                            System.arraycopy(mSpsPps, 0, keyframe, 0, mSpsPps.length);
                                            System.arraycopy(outData, 0, keyframe, mSpsPps.length, outData.length);
                                            mBufferedOutputStream.write(keyframe, 0, keyframe.length);
                                            break;
                                        default:
                                            mBufferedOutputStream.write(outData, 0, outData.length);
                                            break;
                                    }
                                    mMediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                                    outputBufferIndex = mMediaCodec.dequeueOutputBuffer(bufferInfo, mSleepTime);
                                }
                            } catch (Throwable t) {
                                t.printStackTrace();
                            }
                        } else {
                            //队列为空
                            try {
                                Thread.sleep(500);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
            });
            mWork.start();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void onCameraStop(View v) {
        Log.i(TAG, "Stop...");
        try {
            mEncoding = false;
            if (mBufferedOutputStream != null) {
                mBufferedOutputStream.flush();
                mBufferedOutputStream.close();
            }
            if (mWork != null) {
                try {
                    Log.i(TAG, "Stop thread ...");
                    mWork.stop();
                    mWork = null;
                    Log.i(TAG, "...Stop thread");
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (mMediaCodec != null) {
                mMediaCodec.stop();
                mMediaCodec.release();
                mMediaCodec = null;
            }
            if (mCamera != null) {
                mCamera.setPreviewCallbackWithBuffer(null);
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.i(TAG, "...Stop");
    }

    private byte[] NV21ToNV12(byte[] nv21, int width, int height) {
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

    private byte[] Rotate90(byte[] src, int width, int height) {
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
                yuv[i] = src[(width * height) + (y * width)
                        + (x - 1)];
                i--;
            }
        }
        return yuv;
    }

    private byte[] Rotate180(byte[] src, int width, int height) {
        byte[] yuv = new byte[width * height * 3 / 2];
        int i = 0;
        int count = 0;
        for (i = width * height - 1; i >= 0; i--) {
            yuv[count] = src[i];
            count++;
        }
        i = width * height * 3 / 2 - 1;
        for (i = width * height * 3 / 2 - 1; i >= width
                * height; i -= 2) {
            yuv[count++] = src[i - 1];
            yuv[count++] = src[i];
        }
        return yuv;
    }

    private byte[] Rotate270(byte[] src, int width,
                             int height) {
        byte[] yuv = new byte[width * height * 3 / 2];
        int nWidth = 0, nHeight = 0;
        int wh = 0;
        int uvHeight = 0;
        if (width != nWidth || height != nHeight) {
            nWidth = width;
            nHeight = height;
            wh = width * height;
            uvHeight = height >> 1;// uvHeight = height / 2
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
    private void Mirror(byte[] src, int w, int h) { //src是原始yuv数组
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