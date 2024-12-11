package com.gongluck.camera;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.ShutterCallback;
import android.hardware.Camera.PictureCallback;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.EncoderCapabilities;
import android.media.MediaCodecInfo.CodecProfileLevel;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.opengl.GLES11Ext;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

import com.gongluck.helper.CameraHelper;
import com.gongluck.helper.FormatCovertHelper;
import com.gongluck.helper.MediaCodecHelper;
import com.gongluck.helper.MediaMuxerHelper;
import com.gongluck.helper.RotationHelper;

import static android.widget.ImageView.ScaleType;

@RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
public class MainActivity extends AppCompatActivity {

    static final String TAG = "app-camera";

    //相机API封装
    private CameraHelper mCameraHelper = new CameraHelper();

    //MediaCodecAPI封装
    private MediaCodecHelper mMediaCodecHelper = new MediaCodecHelper();
    private byte[] mSpsPps = null;

    //MediaMuxerAPI封装
    private MediaMuxerHelper mMediaMuxerHelper = new MediaMuxerHelper();
    private int mVideoIndex;

    //旋转工具
    private RotationHelper mRotationHelper = new RotationHelper();

    //格式工具
    private FormatCovertHelper mFormatCovertHelper = new FormatCovertHelper();

    //控件
    private SurfaceView mSurfaceView = null;
    private ImageView mImageView = null;
    private Button mStartFront = null;
    private Button mStartFrontSurface = null;
    private Button mStartBack = null;
    private Button mStartBackSurface = null;
    private Button mStop = null;

    //相机图像参数
    private List<Integer> mFormat = new ArrayList<>();
    private int mWidth;
    private int mHeight;

    //数据保存路径
    private String mPreviewCallbackRawPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_preview-callback-raw.yuv";
    private String mPreviewCallbackRawRotationPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_preview-callback-raw-rotation.yuv";
    private String mPreviewCallbackJpgPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_preview-callback-jpg.jpg";
    //拍照文件路径
    private String mTakePictureRawPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_takepicture-raw.yuv";
    private String mTakePicturePostviewPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_takepicture-postview.yuv";
    private String mTakePictureJpegPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_takepicture-jpg.jpg";
    //视频文件路径
    private String mH264Path = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_encode.h264";
    private String mH265Path = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_encode.h265";
    private String mOtherPath = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_encode.other";
    private BufferedOutputStream mBufferedOutputStream = null;
    private String mMP4Path = Environment.getExternalStorageDirectory().getPath() + "/Download/" + TAG + "_encode.mp4";

    //旋转角度
    private int mRotation;

    //是否使用前置摄像头
    private boolean mUsefacing = false;
    //使用SurfaceView预览
    private boolean mUseSurfacePreview = false;
    //编码类型
    private String mMine = MediaFormat.MIMETYPE_VIDEO_HEVC;

    //预览纹理和缓冲区
    private SurfaceTexture mSurfaceTexture = new SurfaceTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
    private byte[] mPreviewData = new byte[1920 * 1920 * 3];

    //权限
    private int mRequestCodeCamera = 1;
    private int mRequestCodeStorage = 2;

    /////////////////////////////////////////////////////////////////////////////////////////////////

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //控件
        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        mImageView = (ImageView) findViewById(R.id.imageView);
        mStartFront = (Button) findViewById(R.id.btn_start_front);
        mStartFrontSurface = (Button) findViewById(R.id.btn_start_front_surface);
        mStartBack = (Button) findViewById(R.id.btn_start_back);
        mStartBackSurface = (Button) findViewById(R.id.btn_start_back_surface);
        mStop = (Button) findViewById(R.id.btn_stop);
        final Activity thiz = this;
        mStartFront.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onOpen(thiz, true, false);
                    }
                }
        );
        mStartFrontSurface.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onOpen(thiz, true, true);
                    }
                }
        );
        mStartBack.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onOpen(thiz, false, false);
                    }
                }
        );
        mStartBackSurface.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onOpen(thiz, false, true);
                    }
                }
        );
        mStop.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onCameraStop();
                    }
                }
        );

        //https://blog.csdn.net/2301_79985012/article/details/138266064
        if (ContextCompat.checkSelfPermission(thiz, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(thiz, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG, "request file permission");
            ActivityCompat.requestPermissions(thiz, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE,}, mRequestCodeStorage);
        }
    }

    // 处理权限请求结果
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults.length > 0) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                if (requestCode == mRequestCodeCamera) {
                    onCameraOpen();
                }
            } else {
                Toast.makeText(this, requestCode + " permission is required for this feature", Toast.LENGTH_SHORT).show();
            }
        }
    }

    void onOpen(Activity context, boolean useFacing, boolean useSurfacePreview) {
        mUsefacing = useFacing;
        mUseSurfacePreview = useSurfacePreview;
        if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG, "request camera permission");
            ActivityCompat.requestPermissions(context, new String[]{Manifest.permission.CAMERA}, mRequestCodeCamera);
        } else {
            onCameraOpen();
        }
    }

    void onCameraOpen() {
        onCameraStop();

        //打开摄像头
        if (!mCameraHelper.Open(mUsefacing)) {
            Log.e(TAG, "open camera device failed");
            return;
        }

        //获取相机支持的参数
        List<Integer> supportedPictureFormats = new ArrayList<>();
        List<Size> supportedPictureSizes = new ArrayList<>();
        List<Integer> supportedPreviewFormats = new ArrayList<>();
        List<Size> supportedPreviewSizes = new ArrayList<>();
        if (!mCameraHelper.GetCameraSupportParams(supportedPictureFormats, supportedPictureSizes, supportedPreviewFormats, supportedPreviewSizes)) {
            Log.e(TAG, "get camera support params failed");
            return;
        }

        for (int f : supportedPictureFormats) {
            Log.i(TAG, "got support picture format: 0x" + Integer.toHexString(f));
        }
        for (Size s : supportedPictureSizes) {
            Log.i(TAG, "got support picture size: " + s.width + " x " + s.height);
            if (s.width <= 1920 && s.height <= 1920 && s.width >= 360 && s.height >= 360) {
                Log.i(TAG, "use size: " + s.width + " x " + s.height);
                mWidth = s.width;
                mHeight = s.height;
            }
        }
        for (int f : supportedPreviewFormats) {
            Log.i(TAG, "got support preview format: 0x" + Integer.toHexString(f));
        }
        for (Size s : supportedPreviewSizes) {
            Log.i(TAG, "got support preview size : " + s.width + " x " + s.height);
            if (s.width <= 1920 && s.height <= 1920 && s.width >= 360 && s.height >= 360) {
                Log.i(TAG, "use size: " + s.width + " x " + s.height);
                mWidth = s.width;
                mHeight = s.height;
            }
        }

        //相机参数
        if (!mCameraHelper.SetPictureParams(ImageFormat.JPEG, mWidth, mHeight)) {
            Log.e(TAG, "set camera picture params failed");
            return;
        }
        if (!mCameraHelper.SetPreviewParams(ImageFormat.NV21, mWidth, mHeight)) {
            Log.e(TAG, "set camera preview params failed");
            return;
        }
        if (!mCameraHelper.SetZoomRotation(0, 0)) {
            Log.e(TAG, "set camera zoom and rotation params failed");
            return;
        }
        if (!mCameraHelper.SetAutoFocus()) {
            Log.e(TAG, "set camera auto focus params failed");
            return;
        }

        //设置相机参数
        if (!mCameraHelper.UpdateParams()) {
            Log.e(TAG, "update camera params failed");
            return;
        }

        //获取相机参数
        List<Integer> width = new ArrayList<>();
        List<Integer> height = new ArrayList<>();
        if (!mCameraHelper.GetCameraParams(mFormat, width, height)) {
            Log.e(TAG, "get camera params failed");
            return;
        }
        mWidth = width.get(0);
        mHeight = height.get(0);

        //预览回调
        if (!mCameraHelper.SetPreviewCallback(mPrewviewCallback)) {
            Log.e(TAG, "set camera preview callback failed");
            return;
        }
        if (!mCameraHelper.AddCallbackBuffer(mPreviewData)) {
            Log.e(TAG, "add camera preview callback buffer failed");
            return;
        }

        //系统旋转角度
        mRotation = mRotationHelper.CaluateRotation(mUsefacing, mCameraHelper.GetCamraOrientation(), this.getWindowManager().getDefaultDisplay().getRotation());
        Log.i(TAG, "rotation: " + mRotation);

        if (mUseSurfacePreview) {
            mImageView.setVisibility(View.INVISIBLE);
            mSurfaceView.setVisibility(View.VISIBLE);

            //预览旋转
            if (!mCameraHelper.SetDisplayOrientation(mRotation)) {
                Log.e(TAG, "set camera display rotation failed");
                return;
            }
            //预览显示
            try {
                if (!mCameraHelper.SetPreviewDisplay(mSurfaceView.getHolder())) {
                    Log.e(TAG, "set camera display view failed");
                    return;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            mSurfaceView.setVisibility(View.INVISIBLE);
            mImageView.setVisibility(View.VISIBLE);

            //预览纹理
            try {
                if (!mCameraHelper.SetPreviewTexture(mSurfaceTexture)) {
                    Log.e(TAG, "set camera display texture failed");
                    return;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        try {
            //开始预览
            if (!mCameraHelper.StartPreview()) {
                Log.e(TAG, "start camera preview failed");
                return;
            }

            //拍照
            takePicture();

            //创建封装
            if (!mMediaMuxerHelper.OpenMux(mMP4Path, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)) {
                Log.e(TAG, "open muxer failed");
                return;
            }

            switch (mMine) {
                case MediaFormat.MIMETYPE_VIDEO_AVC:
                    mBufferedOutputStream = new BufferedOutputStream(new FileOutputStream(new File(mH264Path)));
                    break;
                case MediaFormat.MIMETYPE_VIDEO_HEVC:
                    mBufferedOutputStream = new BufferedOutputStream(new FileOutputStream(new File(mH265Path)));
                    break;
                default:
                    mBufferedOutputStream = new BufferedOutputStream(new FileOutputStream(new File(mOtherPath)));
                    break;
            }

            //获取编解码器信息
            List<MediaCodecInfo> infos = new ArrayList<>();
            mMediaCodecHelper.GetMediaCodecList(infos);
            for (MediaCodecInfo info : infos) {
            }

            //设置编码参数
            int vwidth = mWidth;
            int vheight = mHeight;
            if (mRotation % 180 != 0) {
                vwidth = mHeight;
                vheight = mWidth;
            }
            if (!mMediaCodecHelper.SetVideoEncodeParams(mMine, vwidth, vheight, CodecProfileLevel.AVCProfileBaseline, CodecProfileLevel.AVCLevel1, EncoderCapabilities.BITRATE_MODE_CBR,
                    1000000, 30, 1, CodecCapabilities.COLOR_FormatYUV420SemiPlanar)) {
                Log.e(TAG, "set video encode params failed");
                return;
            }

            if (!mMediaCodecHelper.SetVideoEncodeCallback(mVideoEncodeCallback)) {
                Log.e(TAG, "set video encode callback failed");
                return;
            }

            //开始编码
            if (!mMediaCodecHelper.StartEncodeByType(mMine)) {
                Log.e(TAG, "start encode failed");
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void onCameraStop() {
        Log.i(TAG, "Stop...");
        try {
            mCameraHelper.SetPreviewCallback(null);
            mCameraHelper.StopPreview();
            mCameraHelper.Close();

            mMediaCodecHelper.StopEncode();
            mMediaCodecHelper.Reset();

            if (mBufferedOutputStream != null) {
                mBufferedOutputStream.flush();
                mBufferedOutputStream.close();
            }

            mMediaMuxerHelper.StopMux();
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.i(TAG, "...Stop");
    }

    //采集帧回调
    PreviewCallback mPrewviewCallback = new PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            //设置下一帧回调，调试一天，原来是逻辑错误没有调用addCallbackBuffer导致编码出的264数据不能正常播放，只有一帧有效数据
            //放在最前面，以防忘记！
            if (!mCameraHelper.AddCallbackBuffer(mPreviewData)) {
                Log.e(TAG, "add camera preview callback buffer failed");
                return;
            }

            //Log.d(TAG, "preview frame data length: " + data.length);
            try {
                //保存yuv
                saveToFile(data, mPreviewCallbackRawPath);

                //对数据裁剪旋转到正常显示
                byte[] input = new byte[mWidth * mHeight * 3 / 2];
                System.arraycopy(data, 0, input, 0, input.length);
                int width = mWidth;
                int height = mHeight;
                //Log.d(TAG, "image view size: " + mImageView.getWidth() + " x " + mImageView.getHeight());
                switch (mRotation) {
                    case 90:
                        input = mRotationHelper.Rotate90(input, mWidth, mHeight);
                        width = mHeight;
                        height = mWidth;
                        break;
                    case 180:
                        input = mRotationHelper.Rotate180(input, mWidth, mHeight);
                        break;
                    case 270:
                        input = mRotationHelper.Rotate270(input, mWidth, mHeight);
                        width = mHeight;
                        height = mWidth;
                        break;
                }
                if (mUsefacing) {
                    mRotationHelper.Mirror(input, width, height);
                }

                //保存yuv
                saveToFile(input, mPreviewCallbackRawRotationPath);

                //YuvImage
                YuvImage yuvImg = new YuvImage(input, mFormat.get(0), width, height, null);

                //保存jpeg
                ByteArrayOutputStream stream = new ByteArrayOutputStream();
                yuvImg.compressToJpeg(new Rect(0, 0, yuvImg.getWidth(), yuvImg.getHeight()), 100, stream);
                saveToFile(stream.toByteArray(), mPreviewCallbackJpgPath);

                if (!mUseSurfacePreview) {
                    //显示到imageview
                    Bitmap bm = BitmapFactory.decodeByteArray(stream.toByteArray(), 0, stream.size());
                    mImageView.setImageBitmap(bm);
                    mImageView.setScaleType(ScaleType.FIT_XY);

                    //水平翻转
                    //https://blog.csdn.net/u013394527/article/details/109131769
                    //if (mUsefacing) {
                    //    mImageView.setScaleX(-1);
                    //}

                    //旋转
                    //mImageView.setRotation(mRotation);
                }

                stream.close();

                mFormatCovertHelper.NV21ToNV12(input, width, height);
                //推送待编码数据
                mMediaCodecHelper.InputBuffer(input);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    };

    private void takePicture() {
        //打开预览后才能拍照
        mCameraHelper.TakePicture(
                new ShutterCallback() {
                    @Override
                    public void onShutter() {
                    }
                },
                new PictureCallback() {
                    @Override
                    public void onPictureTaken(byte[] data, Camera camera) {
                        if (data == null) {
                            Log.e(TAG, "raw data is null");
                            return;
                        }
                        try {
                            saveToFile(data, mTakePictureRawPath);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                },
                new PictureCallback() {
                    @Override
                    public void onPictureTaken(byte[] data, Camera camera) {
                        try {
                            saveToFile(data, mTakePicturePostviewPath);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                },
                new PictureCallback() {
                    @Override
                    public void onPictureTaken(byte[] data, Camera camera) {
                        try {
                            //saveToFile(data, mTakePictureJpegPath);

                            Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
                            Matrix matrix = new Matrix();
                            matrix.postRotate(mRotation);
                            bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);

                            //保存jpeg
                            ByteArrayOutputStream stream = new ByteArrayOutputStream();
                            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, stream);
                            saveToFile(stream.toByteArray(), mTakePictureJpegPath);
                            stream.close();
                            bitmap.recycle();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        //恢复预览
                        mCameraHelper.StartPreview();
                    }
                }
        );
    }

    MediaCodecHelper.VideoEncodeCallback mVideoEncodeCallback = new MediaCodecHelper.VideoEncodeCallback() {
        @Override
        public void EncodeCallback(MediaCodec.BufferInfo info, ByteBuffer buffer) {
            try {
                byte[] data = new byte[info.size];
                buffer.get(data);

                if (!mMediaMuxerHelper.WriteSample(mVideoIndex, buffer, info)) {
                    Log.e(TAG, "write video data failed");
                }
                //Log.d(TAG, "timestamp: " + (info.presentationTimeUs / 1000) + "ms");

                switch (info.flags) {
                    case MediaCodec.BUFFER_FLAG_CODEC_CONFIG://sps pps
                        mSpsPps = data;
                        break;
                    case MediaCodec.BUFFER_FLAG_KEY_FRAME://I
                        byte[] keyframe = new byte[info.size + mSpsPps.length];
                        System.arraycopy(mSpsPps, 0, keyframe, 0, mSpsPps.length);
                        System.arraycopy(data, 0, keyframe, mSpsPps.length, info.size);
                        mBufferedOutputStream.write(keyframe, 0, keyframe.length);
                    default:
                        mBufferedOutputStream.write(data, 0, info.size);
                        break;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void OutputFormatChange(MediaFormat format) {
            mVideoIndex = mMediaMuxerHelper.AddTrack(format);
            if (!mMediaMuxerHelper.StartMux()) {
                Log.e(TAG, "start muxer failed");
                return;
            }
        }
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////

    //保存数据
    private void saveToFile(byte[] data, String path) throws IOException {
        File file = new File(path);
        if (file.exists()) {
            file.delete();
        }
        BufferedOutputStream outputStream = new BufferedOutputStream(new FileOutputStream(file));
        outputStream.write(data, 0, data.length);
        outputStream.flush();
        outputStream.close();
    }
}
