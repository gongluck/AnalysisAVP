package com.gongluck.opengles_helper;

import android.content.Context;

import com.gongluck.opengles_java.R;

import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glUniformMatrix4fv;

public class ColorProgramHelper extends ProgramHelper {
    //uniform变量
    private final int uMatrixLocation;
    //attribute变量
    private final int aPositionLocation;
    private final int aColorLocation;

    public ColorProgramHelper(Context context) {
        super(context, R.raw.vertex_shader,
                R.raw.fragment_shader);
        //获取opengl程序中的统一变量索引
        uMatrixLocation = glGetUniformLocation(program, U_MATRIX);
        //获取opengl程序中的属性索引
        aPositionLocation = glGetAttribLocation(program, A_POSITION);
        aColorLocation = glGetAttribLocation(program, A_COLOR);
    }

    public void setUniforms(float[] matrix) {
        //更新u_Matrix
        glUniformMatrix4fv(uMatrixLocation, 1, false, matrix, 0);
    }

    public int getPositionAttributeLocation() {
        return aPositionLocation;
    }

    public int getColorAttributeLocation() {
        return aColorLocation;
    }
}
