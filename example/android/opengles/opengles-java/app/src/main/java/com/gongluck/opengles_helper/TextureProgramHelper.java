package com.gongluck.opengles_helper;

import android.content.Context;

import com.gongluck.opengles_java.R;

import static android.opengl.GLES20.GL_TEXTURE0;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.glActiveTexture;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glUniform1i;
import static android.opengl.GLES20.glUniformMatrix4fv;

public class TextureProgramHelper extends ProgramHelper {
    //uniform变量
    private final int uMatrixLocation;
    private final int uTextureUnitLocation;
    //attribute变量
    private final int aPositionLocation;
    private final int aTextureCoordinatesLocation;

    public TextureProgramHelper(Context context) {
        super(context, R.raw.texture_vertex_shader,
                R.raw.texture_fragment_shader);
        //获取opengl程序中的统一变量索引
        uMatrixLocation = glGetUniformLocation(program, U_MATRIX);
        uTextureUnitLocation = glGetUniformLocation(program, U_TEXTURE_UNIT);
        //获取opengl程序中的属性索引
        aPositionLocation = glGetAttribLocation(program, A_POSITION);
        aTextureCoordinatesLocation =
                glGetAttribLocation(program, A_TEXTURE_COORDINATES);
    }

    public void setUniforms(float[] matrix, int textureId) {
        //更新u_Matrix
        glUniformMatrix4fv(uMatrixLocation, 1, false, matrix, 0);
        //激活第一个纹理单元
        glActiveTexture(GL_TEXTURE0);
        //绑定纹理到当前激活的纹理单元
        glBindTexture(GL_TEXTURE_2D, textureId);
        //更新u_TextureUnit
        glUniform1i(uTextureUnitLocation, 0);//告诉着色器使用哪个纹理单元
    }

    public int getPositionAttributeLocation() {
        return aPositionLocation;
    }

    public int getTextureCoordinatesAttributeLocation() {
        return aTextureCoordinatesLocation;
    }
}