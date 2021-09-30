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

    private int mWidth = 640;
    private int mHeight = 480;

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
            //Log.i(TAG, "data info:\nformat = " + imageFormat + "\nwidth = " + w + "\nheight = " + h);

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

                    mImageView.setRotation(270);
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
        int facingdevice = -1;
        for (int i = 0; i < nums; ++i) {
            Camera.getCameraInfo(i, camerainfo);
            Log.i(TAG, "The " + i + " camera device info:\nfacing=" + camerainfo.facing);
            if (camerainfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                facingdevice = i;
            }
        }

        mCamera = Camera.open(facingdevice);
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
        cameraParams.setRotation(90);
        mCamera.setParameters(cameraParams);

        mCamera.setDisplayOrientation(90);
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
            for(int i = 0; i<counts ;++i){
                MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
                Log.i(TAG, "Support codec : " + info.getName() +
                        ", isencoder : " + info.isEncoder() +
                        ", ishwaccel : " + info.isHardwareAccelerated() +
                        ", issoftware : " + info.isSoftwareOnly());
                for(String type : info.getSupportedTypes()){
                    Log.i(TAG, "\tsupport type : " + type);
                    if(type.equals(MediaFormat.MIMETYPE_VIDEO_AVC) && info.isEncoder() && info.isHardwareAccelerated()){
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
                            byte[] rotate = new byte[mWidth * mHeight * 3 / 2];
                            byte[] nv21 = new byte[mWidth * mHeight * 3 / 2];
                            NV21_rotate_to_90(input, rotate, mWidth, mHeight);
                            NV21ToNV12(rotate, nv21, mHeight, mWidth);
                            input = nv21;
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

    private byte[] NV21_rotate_to_90(byte[] nv21_data, byte[] nv21_rotated, int width, int height)
    {
        int y_size = width * height;
        int buffser_size = y_size * 3 / 2;

        // Rotate the Y luma
        int i = 0;
        int startPos = (height - 1)*width;
        for (int x = 0; x < width; x++)
        {
            int offset = startPos;
            for (int y = height - 1; y >= 0; y--)
            {
                nv21_rotated[i] = nv21_data[offset + x];
                i++;
                offset -= width;
            }
        }

        // Rotate the U and V color components
        i = buffser_size - 1;
        for (int x = width - 1; x > 0; x = x - 2)
        {
            int offset = y_size;
            for (int y = 0; y < height / 2; y++)
            {
                nv21_rotated[i] = nv21_data[offset + x];
                i--;
                nv21_rotated[i] = nv21_data[offset + (x - 1)];
                i--;
                offset += width;
            }
        }
        return nv21_rotated;
    }

    private void NV21ToNV12(byte[] nv21, byte[] nv12, int width, int height) {
        if (nv21 == null || nv12 == null) {
            return;
        }
        int framesize = width * height;
        int i = 0, j = 0;
        //System.arraycopy(nv21, 0, nv12, 0, framesize);
        for (i = 0; i < framesize; i++) {
            nv12[i] = nv21[i];
        }
        for (j = 0; j < framesize / 2; j += 2) {
            nv12[framesize + j - 1] = nv21[j + framesize];
        }
        for (j = 0; j < framesize / 2; j += 2) {
            nv12[framesize + j] = nv21[j + framesize - 1];
        }
    }
}