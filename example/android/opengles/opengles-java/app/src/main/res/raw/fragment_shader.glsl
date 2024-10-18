precision mediump float;//定义float类型默认精度

//uniform变量在着色器的所有实例中共享的值
//一个uniform的位置在一个程序对象中是唯一的
uniform vec4 _Color;

void main()
{
    //输出颜色
    gl_FragColor = _Color;
}