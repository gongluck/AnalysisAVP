package com.gongluck.capture;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import com.gongluck.helper.CaptureHelper;
import com.gongluck.helper.FormatCovertHelper;
import com.gongluck.helper.PermissionHelper;

import java.nio.ByteBuffer;

@RequiresApi(api = Build.VERSION_CODES.O)
public class MainActivity extends AppCompatActivity {

    static final String TAG = "app-capture";

    //屏幕采集工具
    private CaptureHelper mCaptureHelper = new CaptureHelper();

    //格式工具
    private FormatCovertHelper mFormatCovertHelper = new FormatCovertHelper();

    //权限工具
    private PermissionHelper mPermissionHelper = new PermissionHelper();

    //图像参数
    private int mWidth = 720;
    private int mHeight = 1280;

    //采集保存图片
    private ImageReader mImageReader;
    private long mImageCount;
    private long mLastTimeMs;
    private long mCalFpsIntervalMs = 10000;

    //数据保存路径
    private String mCaptureRawPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_capture-raw.rgb";

    //权限
    private int mRequestCodeCapture = 1;
    private int mRequestCodeStorage = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mPermissionHelper.CheckPermission(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, mRequestCodeStorage);
        mPermissionHelper.CheckScreenCapturePermission(this, mRequestCodeCapture);
    }

    // 处理权限请求结果
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == mRequestCodeCapture && resultCode == Activity.RESULT_OK) {
            Toast.makeText(this,  "screen capture required succeed", Toast.LENGTH_SHORT).show();

            final Context thiz = this;

            mLastTimeMs = System.currentTimeMillis();
            mImageCount = 0;
            mImageReader = ImageReader.newInstance(mWidth, mHeight, PixelFormat.RGBA_8888, 2);
            mImageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
                @Override
                public void onImageAvailable(ImageReader reader) {
                    Image image = reader.acquireLatestImage();
                    if (image != null) {
                        Image.Plane[] planes = image.getPlanes();
                        if (planes.length > 0) {
                            Image.Plane plane = planes[0];
                            ByteBuffer buffer = plane.getBuffer();

                            byte[] data = new byte[buffer.remaining()];
                            buffer.get(data);
                            byte[] dataNoStride = new byte[data.length];
                            mFormatCovertHelper.CutStride(data, mWidth, mHeight, plane.getPixelStride(), plane.getRowStride() / plane.getPixelStride(), dataNoStride);
                            mFormatCovertHelper.SaveFile(dataNoStride, mCaptureRawPath);

                            ++mImageCount;
                            long now = System.currentTimeMillis();
                            if(now - mLastTimeMs > mCalFpsIntervalMs) {
                                final long fps = mImageCount * 1000 / (now - mLastTimeMs);
                                new Handler(Looper.getMainLooper()).post(new Runnable() {
                                    @Override
                                    public void run() {
                                        Toast.makeText(thiz, "fps: " + fps, Toast.LENGTH_SHORT).show();
                                    }
                                });
                                mImageCount = 0;
                                mLastTimeMs = now;
                            }
                        }
                        image.close();
                    }
                }
            }, null);

            VirtualDisplay[] virtualDisplay = new VirtualDisplay[1];
            mCaptureHelper.StartScreenCapture(this, resultCode, data, "screen capture", mWidth, mHeight, 400, DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC,
                    mImageReader.getSurface(), null, null, virtualDisplay);

            moveTaskToBack(true);
        }
    }

    // 处理权限请求结果
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults.length > 0) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this,  "[" + String.join(", ", permissions) + "] required succeed", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, requestCode + " permission is required for this feature", Toast.LENGTH_SHORT).show();
            }
        }
    }
}