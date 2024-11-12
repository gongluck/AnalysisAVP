package com.gongluck.opengles_helper;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import static android.opengl.GLES20.GL_LINEAR;
import static android.opengl.GLES20.GL_LINEAR_MIPMAP_LINEAR;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_TEXTURE_MAG_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_MIN_FILTER;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glDeleteTextures;
import static android.opengl.GLES20.glGenTextures;
import static android.opengl.GLES20.glGenerateMipmap;
import static android.opengl.GLES20.glTexParameteri;
import static android.opengl.GLUtils.texImage2D;

public class TextureHelper {

    private static final String TAG = "TextureHelper";

    public static int loadTexture(Context context, int resourceId) {
        final int[] textureObjectIds = new int[1];
        //创建纹理对象
        glGenTextures(1, textureObjectIds, 0);
        if (textureObjectIds[0] == 0) {
            Log.w(TAG, "Could not generate a new OpenGL texture object.");
            return 0;
        }

        final BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        final Bitmap bitmap = BitmapFactory.decodeResource(
                context.getResources(), resourceId, options);
        if (bitmap == null) {
            Log.w(TAG, "Resource ID " + resourceId + " could not be decoded.");
            glDeleteTextures(1, textureObjectIds, 0);
            return 0;
        }

        //绑定纹理对象
        glBindTexture(GL_TEXTURE_2D, textureObjectIds[0]);
        //设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER/*缩小时*/, GL_LINEAR_MIPMAP_LINEAR/*mipmap线性过滤*/);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER/*放大时*/, GL_LINEAR/*线性过滤*/);
        //读取数据到纹理对象
        texImage2D(GL_TEXTURE_2D, 0, bitmap, 0);
        //当前纹理对象生成mipmap
        glGenerateMipmap(GL_TEXTURE_2D);
        //释放图片资源
        bitmap.recycle();
        //还原绑定纹理
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureObjectIds[0];
    }
}
