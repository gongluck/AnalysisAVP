precision mediump float;//定义float类型默认精度

//uniform变量在着色器的所有实例中共享的值
//一个uniform的位置在一个程序对象中是唯一的
//uniform vec4 _Color;

//varying用于在顶点着色器和片元着色器之间传递数据
//varying变量可以包含顶点着色器计算的数据,会在光栅化过程中被插值,然后在片段着色器中使用
varying vec4 v_Color;//插值变量

void main()
{
    //输出颜色
    //gl_FragColor = _Color;
    gl_FragColor = v_Color;
}