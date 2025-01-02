package com.gongluck.helper;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import static android.content.Context.MEDIA_PROJECTION_SERVICE;

@RequiresApi(api = Build.VERSION_CODES.O)
public class PermissionHelper {

    static final String TAG = "PermissionHelper";

    //获取权限
    public boolean CheckPermission(Activity activity, String[] permissions, int requestCode) {
        //https://blog.csdn.net/2301_79985012/article/details/138266064
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(activity, permission) != PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "request permission: " + permission + " in [" + String.join(", ", permissions) + "]");
                //void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults); 获取权限请求结果
                ActivityCompat.requestPermissions(activity, permissions, requestCode);
                return false;
            }
        }

        return true;
    }

    //获取屏幕采集权限
    public boolean CheckScreenCapturePermission(Activity activity, int requestCode) {
        MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) activity.getSystemService(MEDIA_PROJECTION_SERVICE);
        Intent intent = mediaProjectionManager.createScreenCaptureIntent();
        activity.startActivityForResult(intent, requestCode);

        return true;
    }
}
