package com.gongluck.camera;

import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.ShutterCallback;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.SurfaceHolder;

import java.io.IOException;
import java.util.List;

public class CameraHelper {

    static final String TAG = "CameraHelper";

    private Camera mCamera;
    private boolean mFacing;
    private Integer mDeviceId = -1;
    private CameraInfo mCameraInfo;
    private Parameters mParameters;
    private boolean mPreviewing;

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //打开摄像头
    public boolean Open(boolean facing) {
        Close();

        mFacing = facing;

        mCameraInfo = new CameraInfo();
        if (!getDevice(mFacing, mCameraInfo)) {
            Log.e(TAG, "can not find device for type.");
            return false;
        }
        Log.i(TAG, "open device id: " + mDeviceId);
        mCamera = Camera.open(mDeviceId);
        mParameters = mCamera.getParameters();

        return true;
    }

    //关闭摄像头
    public boolean Close() {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        StopPreview();
        mCamera.release();
        mCamera = null;
        mParameters = null;
        mCameraInfo = null;

        return true;
    }

    //开始预览
    public boolean StartPreview() {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        mCamera.startPreview();
        mPreviewing = true;

        return true;
    }

    //停止预览预览
    public boolean StopPreview() {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        mCamera.stopPreview();
        mPreviewing = false;

        return true;
    }

    //拍照
    public boolean TakePicture(ShutterCallback shutter, PictureCallback raw, PictureCallback postview, PictureCallback jpeg) {
        if (mCamera == null || !mPreviewing) { //打开预览后才能拍照
            Log.e(TAG, "have not previewing.");
            return false;
        }

        mCamera.takePicture(shutter, raw, postview, jpeg);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //获取相机旋转
    public int GetCamraOrientation() {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return 0;
        }

        return mCameraInfo.orientation;
    }

    //获取相机支持的参数
    public boolean GetCameraSupportParams(List<Integer> supportedPictureFormats, List<Size> supportedPictureSizes, List<Integer> supportedPreviewFormats, List<Size> supportedPreviewSizes) {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        supportedPictureFormats.clear();
        supportedPictureFormats.addAll(mParameters.getSupportedPictureFormats());
        for (int f : supportedPictureFormats) {
            Log.i(TAG, "support picture format: 0x" + Integer.toHexString(f));
        }

        supportedPictureSizes.clear();
        supportedPictureSizes.addAll(mParameters.getSupportedPictureSizes());
        for (Size s : supportedPictureSizes) {
            Log.i(TAG, "support picture size: " + s.width + " x " + s.height);
        }

        supportedPreviewFormats.clear();
        supportedPreviewFormats.addAll(mParameters.getSupportedPreviewFormats());
        for (int f : supportedPreviewFormats) {
            Log.i(TAG, "support preview format: 0x" + Integer.toHexString(f));
        }

        supportedPreviewSizes.clear();
        supportedPreviewSizes.addAll(mParameters.getSupportedPreviewSizes());
        for (Size s : supportedPreviewSizes) {
            Log.i(TAG, "support preview size : " + s.width + " x " + s.height);
        }

        return true;
    }

    //设置图片参数
    public boolean SetPictureParams(int format, int width, int height) {
        if (mParameters == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        Log.i(TAG, "set picture format: 0x" + Integer.toHexString(format) + ", size: " + width + "x" + height);
        mParameters.setPictureFormat(format);
        mParameters.setPictureSize(width, height); //拍照回调数据的宽高

        return true;
    }

    //设置预览参数
    public boolean SetPreviewParams(int format, int width, int height) {
        if (mParameters == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        Log.i(TAG, "set preview format: 0x" + Integer.toHexString(format) + ", size: " + width + "x" + height);
        mParameters.setPreviewFormat(format); //基本都支持NV21
        mParameters.setPreviewSize(width, height); //预览回调数据宽高

        return true;
    }

    //设置焦距旋转参数
    public boolean SetZoomRotation(int zoom, int rotation) {
        if (mParameters == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        Log.d(TAG, "set zoom: " + zoom + ", rotation: " + rotation);
        mParameters.setZoom(zoom); //焦距
        mParameters.setRotation(rotation); //旋转?

        return true;
    }

    //设置自动对焦参数
    public boolean SetAutoFocus() {
        if (mParameters == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        List<String> focusModes = mParameters.getSupportedFocusModes();
        if (focusModes.contains(Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
            mParameters.setFocusMode(Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
        }

        return true;
    }

    //设置相机参数
    public boolean UpdateParams() {
        if (mCamera == null || mParameters == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        mCamera.setParameters(mParameters);

        return true;
    }

    //获取相机参数
    public boolean GetCameraParams(List<Integer> format, List<Integer> width, List<Integer> height) {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        format.clear();
        width.clear();
        height.clear();

        Parameters parameters = mCamera.getParameters();
        Log.i(TAG, "parameters: " + parameters.flatten());
        format.add(parameters.getPreviewFormat());
        width.add(parameters.getPreviewSize().width);
        height.add(parameters.getPreviewSize().height);

        return true;
    }

    //设置预览旋转
    public boolean SetDisplayOrientation(int rotation) {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        mCamera.setDisplayOrientation(rotation);

        return true;
    }

    //设置预览回调
    public boolean SetPreviewCallback(PreviewCallback cb) {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        //https://www.jianshu.com/p/9a51270b69ea
        mCamera.setPreviewCallbackWithBuffer(cb);

        return true;
    }

    //设置预览回调缓冲区
    public boolean AddCallbackBuffer(byte[] buffer) {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        mCamera.addCallbackBuffer(buffer);

        return true;
    }

    //设置预览显示
    public boolean SetPreviewDisplay(SurfaceHolder holder) throws IOException {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        //https://blog.csdn.net/import_sadaharu/article/details/52744899?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_title~default-0.no_search_link&spm=1001.2101.3001.4242
        mCamera.setPreviewDisplay(holder);

        return true;
    }

    //设置预览纹理
    public boolean SetPreviewTexture(SurfaceTexture texture) throws IOException {
        if (mCamera == null) {
            Log.e(TAG, "have not opened camera device.");
            return false;
        }

        mCamera.setPreviewTexture(texture);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    private boolean getDevice(boolean facing, CameraInfo cameraInfo) {
        int type = facing ? CameraInfo.CAMERA_FACING_FRONT : CameraInfo.CAMERA_FACING_BACK;
        //遍历相机设备
        int nums = Camera.getNumberOfCameras();
        Log.i(TAG, "there has " + nums + " camera devices.");
        mDeviceId = -1;
        for (int i = 0; i < nums; ++i) {
            Camera.getCameraInfo(i, cameraInfo);
            Log.i(TAG, "the " + i + " camera device info:\nfacing=" + cameraInfo.facing);
            if (cameraInfo.facing == type) {
                mDeviceId = i;
                Log.d("TAG", "maybe use device " + i);
                //break;
            }
        }

        return mDeviceId != -1;
    }
}
