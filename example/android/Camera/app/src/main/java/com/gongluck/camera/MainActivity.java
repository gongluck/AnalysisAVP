//参考：
//https://www.jianshu.com/p/9a51270b69ea
//https://blog.csdn.net/import_sadaharu/article/details/52744899?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_title~default-0.no_search_link&spm=1001.2101.3001.4242
package com.gongluck.camera;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.Camera;
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
import java.util.List;

public class MainActivity extends AppCompatActivity {
    static final String TAG = "Camera";
    private SurfaceView mSurfaceView = null;
    private ImageView mImageView = null;
    private Button mStart = null;
    private Button mStop = null;
    private Button mHide = null;
    private Button mShow = null;
    private Camera mCamera = null;
    private SurfaceTexture mSurfaceTexture = new SurfaceTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
    private byte[] mPreviewData = new byte[640 * 480 * 3 / 2];
    private boolean mUseSurfacePreview = true;

    //帧回调
    Camera.PreviewCallback mPrewviewCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            Camera.Parameters parameters = mCamera.getParameters();
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

                    mImageView.setRotation(270);

                    //设置下一帧回调
                    mCamera.addCallbackBuffer(mPreviewData);
                }
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
        cameraParams.setPreviewSize(640, 480);
        cameraParams.setPictureFormat(ImageFormat.NV21);
        cameraParams.setPictureSize(640, 480);
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
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void onCameraStop(View v) {
        Log.i(TAG, "Stop...");
        if (mCamera != null) {
            mCamera.setPreviewCallbackWithBuffer(null);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }
}