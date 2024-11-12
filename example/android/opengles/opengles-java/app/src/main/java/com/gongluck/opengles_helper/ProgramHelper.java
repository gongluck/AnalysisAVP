package com.gongluck.opengles_helper;

import android.content.Context;

import static android.opengl.GLES20.glUseProgram;

public class ProgramHelper {
    //uniform变量
    protected static final String U_MATRIX = "u_Matrix";
    protected static final String U_TEXTURE_UNIT = "u_TextureUnit";
    //attribute变量
    protected static final String A_POSITION = "a_Position";
    protected static final String A_COLOR = "a_Color";
    protected static final String A_TEXTURE_COORDINATES = "a_TextureCoordinates";

    protected final int program;

    protected ProgramHelper(Context context, int vertexShaderResourceId,
                            int fragmentShaderResourceId) {
        program = ShaderHelper.buildProgram(
                BaseHelper.readTextFileFromRawResource(
                        context, vertexShaderResourceId),
                BaseHelper.readTextFileFromRawResource(
                        context, fragmentShaderResourceId));
    }

    public void useProgram() {
        //使用opengl程序
        glUseProgram(program);
    }
}
