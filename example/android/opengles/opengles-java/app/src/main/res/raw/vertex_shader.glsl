attribute vec4 _Position;//当前顶点位置属性参数
attribute vec4 _Color;//顶点颜色属性

//varying用于在顶点着色器和片元着色器之间传递数据
//varying变量可以包含顶点着色器计算的数据,会在光栅化过程中被插值,然后在片段着色器中使用
varying vec4 v_Color;//插值变量

void main()
{
    //输出位置
    gl_Position = _Position;
    //输出点大小
    gl_PointSize = 10.0;

    v_Color = _Color;
}