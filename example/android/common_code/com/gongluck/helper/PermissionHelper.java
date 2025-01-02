package com.gongluck.helper;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

@RequiresApi(api = Build.VERSION_CODES.O)
public class PermissionHelper {

    static final String TAG = "PermissionHelper";

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
}
