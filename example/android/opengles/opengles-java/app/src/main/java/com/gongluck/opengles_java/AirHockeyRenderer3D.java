package com.gongluck.opengles_java;

import android.content.Context;
import android.opengl.GLSurfaceView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static android.opengl.GLES20.*;
import static android.opengl.Matrix.*;

public class AirHockeyRenderer3D implements GLSurfaceView.Renderer {

    private final Context context;
    private static final int POSITION_COMPONENT_COUNT = 2;//每个坐标位置包含2个值 xy
    private static final int COLOR_COMPONENT_COUNT = 3;//每个坐标颜色包含3个值
    private static final int BYTES_PER_FLOAT = 4;
    private static final int STRIDE = //每个顶点的数据占用
            (POSITION_COMPONENT_COUNT + COLOR_COMPONENT_COUNT) * BYTES_PER_FLOAT;
    private FloatBuffer vertexData;//本地堆

    private int program;
    private static final String A_POSITION = "a_Position";
    private int positionLocation;
    private static final String A_COLOR = "a_Color";
    private int colorLocation;
    private static final String U_MATRIX = "u_Matrix";
    private final float[] projectionMatrix = new float[16];
    private int uMatrixLocation;
    private final float[] modelMatrix = new float[16];

    public AirHockeyRenderer3D(Context context) {
        this.context = context;

        float[] tableVerticesWithTriangles = {//opengl坐标横向范围[-1,1]从左到右,纵向范围[-1,1]从下到上
                //三角形的顶点顺序以逆时针排列，叫做 “卷曲顺序”，原因统一使用卷曲顺序有利于 OpenGL 的优化，可以减少绘制那些无法被看到的图形

                // X, Y, R, G, B

                //三角形扇顶点
                0f, 0f, 1f, 1f, 1f,
                -0.5f, -0.8f, 0.7f, 0.7f, 0.7f,
                0.5f, -0.8f, 0.7f, 0.7f, 0.7f,
                0.5f, 0.8f, 0.7f, 0.7f, 0.7f,
                -0.5f, 0.8f, 0.7f, 0.7f, 0.7f,
                -0.5f, -0.8f, 0.7f, 0.7f, 0.7f,

                //线
                -0.5f, 0f, 1f, 0f, 0f,
                0.5f, 0f, 0f, 1f, 0f,

                //椎
                0f, -0.4f, 0f, 0f, 1f,
                0f, 0.4f, 1f, 0f, 0f,
        };
        vertexData = ByteBuffer.allocateDirect(tableVerticesWithTriangles.length * BYTES_PER_FLOAT).order(ByteOrder.nativeOrder()).asFloatBuffer();
        vertexData.put(tableVerticesWithTriangles);
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        String vertexShaderSource = Utils.readTextFileFromRawResource(context, R.raw.vertex_shader);
        String fragmentShaderSource = Utils.readTextFileFromRawResource(context, R.raw.fragment_shader);

        //编译opengl着色器代码
        int vertexShader = Utils.compileVertexShader(vertexShaderSource);
        int fragmentShader = Utils.compileFragmentShader(fragmentShaderSource);

        //链接opengl程序
        program = Utils.linkProgram(vertexShader, fragmentShader);

        //验证opengl程序
        Utils.validateProgram(program);

        //使用opengl程序
        glUseProgram(program);

        //获取opengl程序中的属性索引
        positionLocation = glGetAttribLocation(program, A_POSITION);

        //获取opengl程序中的统一变量索引
        //colorLocation = glGetUniformLocation(program, A_COLOR);

        //获取opengl程序中的属性索引
        colorLocation = glGetAttribLocation(program, A_COLOR);

        vertexData.position(0);
        //传递_Position的数据
        glVertexAttribPointer(positionLocation, POSITION_COMPONENT_COUNT, GL_FLOAT, false, STRIDE, vertexData);
        //使能_Position属性
        glEnableVertexAttribArray(positionLocation);

        vertexData.position(POSITION_COMPONENT_COUNT);
        //传递v_Color属性
        glVertexAttribPointer(colorLocation, COLOR_COMPONENT_COUNT, GL_FLOAT, false, STRIDE, vertexData);
        //使能v_Color属性
        glEnableVertexAttribArray(colorLocation);

        //获取opengl程序中的统一变量索引
        uMatrixLocation = glGetUniformLocation(program, U_MATRIX);
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int width, int height) {
        //设置视口范围
        glViewport(0, 0, width, height);

//        final float aspectRatio = width > height ? (float) width / (float) height : (float) height / (float) width;
//        //创建正交投影矩阵
//        //orthoM(projectionMatrix, 0, -1f, 1f, -1f, 1f, -1f, 1f);//放大x轴
//        if (width > height) {
//            orthoM(projectionMatrix, 0, -aspectRatio, aspectRatio, -1f, 1f, -1f, 1f);//放大x轴
//        } else {
//            orthoM(projectionMatrix, 0, -1f, 1f, -aspectRatio, aspectRatio, -1f, 1f);//放大y轴
//        }
//

        //创建透视投影矩阵
        perspectiveM(projectionMatrix, 0, 45, (float) width / (float) height, 1f, 10f);

        //设置单位矩阵
        setIdentityM(modelMatrix, 0);

        //矩阵运算
        translateM(modelMatrix, 0, 0, 0, -3f); //z坐标-3

        //旋转矩阵
        rotateM(modelMatrix, 0, -60, 1, 0, 0);

        final float[] temp = new float[16];
        //矩阵乘法
        multiplyMM(temp, 0, projectionMatrix, 0, modelMatrix, 0);//乘法顺序 projectionMatrix·modelMatrix·position
        System.arraycopy(temp, 0, projectionMatrix, 0, temp.length);

        //更新u_Matrix
        glUniformMatrix4fv(uMatrixLocation, 1, false, projectionMatrix, 0);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        //将缓存清除为预先的设置值
        glClear(GL_COLOR_BUFFER_BIT);

        //更新u_Color
        //glUniform4f(colorLocation, 1.0f, 1.0f, 1.0f, 1.0f);
        //绘制
        glDrawArrays(GL_TRIANGLE_FAN, 0, 6);//从0-5共6个点4个三角形形成三角形扇

        //画线
        //glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, 6, 2);//从6-7共2个点1条线

        //画点
        //glUniform4f(colorLocation, 0.0f, 0.0f, 1.0f, 1.0f);
        glDrawArrays(GL_POINTS, 8, 1);
        //glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_POINTS, 9, 1);
    }
}
