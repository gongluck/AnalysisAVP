package com.gongluck.opengles_helper;

public class ShaderHelper extends BaseHelper {

    public static int buildProgram(String vertexShaderSource,
                                   String fragmentShaderSource) {
        int program;

        int vertexShader = compileVertexShader(vertexShaderSource);
        int fragmentShader = compileFragmentShader(fragmentShaderSource);

        program = linkProgram(vertexShader, fragmentShader);

        validateProgram(program);

        return program;
    }
}
