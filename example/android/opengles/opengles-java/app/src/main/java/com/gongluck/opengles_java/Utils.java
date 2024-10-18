package com.gongluck.opengles_java;

import android.content.Context;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import static android.opengl.GLES20.*;

public class Utils {

    private static final String TAG = "Utils";

    public static String readTextFileFromRawResource(final Context context,
                                                     final int resourceId) {
        final InputStream inputStream = context.getResources().openRawResource(
                resourceId);
        final InputStreamReader inputStreamReader = new InputStreamReader(
                inputStream);
        final BufferedReader bufferedReader = new BufferedReader(
                inputStreamReader);

        String nextLine;
        final StringBuilder body = new StringBuilder();

        try {
            while ((nextLine = bufferedReader.readLine()) != null) {
                body.append(nextLine);
                body.append('\n');
            }
        } catch (IOException e) {
            return null;
        }

        return body.toString();
    }

    public static int compileVertexShader(String shaderCode) {
        return compileShader(GL_VERTEX_SHADER, shaderCode);
    }

    public static int compileFragmentShader(String shaderCode) {
        return compileShader(GL_FRAGMENT_SHADER, shaderCode);
    }

    private static int compileShader(int type, String shaderCode) {
        //创建着色器
        final int shaderObjectId = glCreateShader(type);
        if (shaderObjectId == 0) {
            Log.e(TAG, "create shader error.");
            return 0;
        }

        //提交着色器源码
        glShaderSource(shaderObjectId, shaderCode);
        //编译着色器源码
        glCompileShader(shaderObjectId);

        //获取编译状态
        final int[] compileStatus = new int[1];
        glGetShaderiv(shaderObjectId, GL_COMPILE_STATUS, compileStatus, 0);
        if (compileStatus[0] == 0) {
            Log.e(TAG, "compile shader error: " + glGetShaderInfoLog(shaderObjectId));
            glDeleteShader(shaderObjectId);
            return 0;
        }

        return shaderObjectId;
    }

    public static int linkProgram(int vertexShaderId, int fragmentShaderId) {
        //创建程序
        final int programObjectId = glCreateProgram();
        if (programObjectId == 0) {
            Log.e(TAG, "create program error.");
            return 0;
        }

        //附加着色器到程序
        glAttachShader(programObjectId, vertexShaderId);
        glAttachShader(programObjectId, fragmentShaderId);

        //链接程序
        glLinkProgram(programObjectId);

        //获取链接状态
        final int[] linkStatus = new int[1];
        glGetProgramiv(programObjectId, GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] == 0) {
            Log.e(TAG, "link program error: " + glGetProgramInfoLog(programObjectId));
            glDeleteProgram(programObjectId);
            return 0;
        }

        return programObjectId;
    }

    public static boolean validateProgram(int programObjectId) {
        //验证程序 验证过程生成的信息将存储在程序的信息日志中
        glValidateProgram(programObjectId);
        //获取程序验证信息
        final int[] validateStatus = new int[1];
        glGetProgramiv(programObjectId, GL_VALIDATE_STATUS, validateStatus, 0);
        Log.v(TAG, "result of validate program: " + validateStatus[0] + "\nLog:\n"
                + glGetProgramInfoLog(programObjectId));

        return validateStatus[0] != 0;
    }
}
