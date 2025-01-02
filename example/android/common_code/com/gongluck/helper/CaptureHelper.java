package com.gongluck.helper;

import android.app.Activity;
import android.content.Intent;
import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Handler;
import android.view.Surface;

import static android.content.Context.MEDIA_PROJECTION_SERVICE;

public class CaptureHelper {

    static final String TAG = "CaptureHelper";

    //开始采集屏幕
    public boolean StartScreenCapture(Activity activity, int permissionResultCode, Intent permissionResultData, String displayName, int width, int height, int dpi, int flags,
                                      Surface surface, VirtualDisplay.Callback callback, Handler handler, VirtualDisplay[] virtualDisplay) {
        MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) activity.getSystemService(MEDIA_PROJECTION_SERVICE);
        MediaProjection mediaProjection = mediaProjectionManager.getMediaProjection(permissionResultCode, permissionResultData);
        virtualDisplay[0] = mediaProjection.createVirtualDisplay(displayName, width, height, dpi, flags, surface, callback, handler);

        return true;
    }
}
