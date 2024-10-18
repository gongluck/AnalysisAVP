attribute vec4 _Position;//当前顶点位置属性参数

void main()
{
    //输出位置
    gl_Position = _Position;
    //输出点大小
    gl_PointSize = 10.0;
}