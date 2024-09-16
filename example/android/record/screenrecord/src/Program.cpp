/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ScreenRecord"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "Program.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <assert.h>

using namespace android;

// 4x4 identity matrix
const float Program::kIdentity[] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
};

// Simple vertex shader.  Texture coord calc includes matrix for GLConsumer
// transform.
static const char* kVertexShader =
        "uniform mat4 uMVPMatrix;\n"
        "uniform mat4 uGLCMatrix;\n"
        "attribute vec4 aPosition;\n"
        "attribute vec4 aTextureCoord;\n"
        "varying vec2 vTextureCoord;\n"
        "void main() {\n"
        "    gl_Position = uMVPMatrix * aPosition;\n"
        "    vTextureCoord = (uGLCMatrix * aTextureCoord).xy;\n"
        "}\n";

// Trivial fragment shader for external texture.
static const char* kExtFragmentShader =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "varying vec2 vTextureCoord;\n"
        "uniform samplerExternalOES uTexture;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(uTexture, vTextureCoord);\n"
        "}\n";

// Trivial fragment shader for mundane texture.
static const char* kFragmentShader =
        "precision mediump float;\n"
        "varying vec2 vTextureCoord;\n"
        "uniform sampler2D uTexture;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(uTexture, vTextureCoord);\n"
        //"    gl_FragColor = vec4(0.2, 1.0, 0.2, 1.0);\n"
        "}\n";

status_t Program::setup(ProgramType type) {
    ALOGV("Program::setup type=%d", type);
    status_t err;

    mProgramType = type;

    GLuint program;
    if (type == PROGRAM_TEXTURE_2D) {
        err = createProgram(&program, kVertexShader, kFragmentShader);
    } else {
        err = createProgram(&program, kVertexShader, kExtFragmentShader);
    }
    if (err != NO_ERROR) {
        return err;
    }
    assert(program != 0);

    maPositionLoc = glGetAttribLocation(program, "aPosition");
    maTextureCoordLoc = glGetAttribLocation(program, "aTextureCoord");
    muMVPMatrixLoc = glGetUniformLocation(program, "uMVPMatrix");
    muGLCMatrixLoc = glGetUniformLocation(program, "uGLCMatrix");
    muTextureLoc = glGetUniformLocation(program, "uTexture");
    if ((maPositionLoc | maTextureCoordLoc | muMVPMatrixLoc |
            muGLCMatrixLoc | muTextureLoc) == -1) {
        ALOGE("Attrib/uniform lookup failed: %#x", glGetError());
        glDeleteProgram(program);
        return UNKNOWN_ERROR;
    }

    mProgram = program;
    return NO_ERROR;
}

void Program::release() {
    ALOGV("Program::release");
    if (mProgram != 0) {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
}

status_t Program::createProgram(GLuint* outPgm, const char* vertexShader,
        const char* fragmentShader) {
    GLuint vs, fs;
    status_t err;

    err = compileShader(GL_VERTEX_SHADER, vertexShader, &vs);
    if (err != NO_ERROR) {
        return err;
    }
    err = compileShader(GL_FRAGMENT_SHADER, fragmentShader, &fs);
    if (err != NO_ERROR) {
        glDeleteShader(vs);
        return err;
    }

    GLuint program;
    err = linkShaderProgram(vs, fs, &program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (err == NO_ERROR) {
        *outPgm = program;
    }
    return err;
}

status_t Program::compileShader(GLenum shaderType, const char* src,
        GLuint* outShader) {
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        ALOGE("glCreateShader error: %#x", glGetError());
        return UNKNOWN_ERROR;
    }

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        ALOGE("Compile of shader type %d failed", shaderType);
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = new char[infoLen];
            if (buf) {
                glGetShaderInfoLog(shader, infoLen, NULL, buf);
                ALOGE("Compile log: %s", buf);
                delete[] buf;
            }
        }
        glDeleteShader(shader);
        return UNKNOWN_ERROR;
    }
    *outShader = shader;
    return NO_ERROR;
}

status_t Program::linkShaderProgram(GLuint vs, GLuint fs, GLuint* outPgm) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        ALOGE("glCreateProgram error: %#x", glGetError());
        return UNKNOWN_ERROR;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        ALOGE("glLinkProgram failed");
        GLint bufLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
            char* buf = new char[bufLength];
            if (buf) {
                glGetProgramInfoLog(program, bufLength, NULL, buf);
                ALOGE("Link log: %s", buf);
                delete[] buf;
            }
        }
        glDeleteProgram(program);
        return UNKNOWN_ERROR;
    }

    *outPgm = program;
    return NO_ERROR;
}



status_t Program::blit(GLuint texName, const float* texMatrix,
        int32_t x, int32_t y, int32_t w, int32_t h, bool invert) const {
    ALOGV("Program::blit %d xy=%d,%d wh=%d,%d", texName, x, y, w, h);

    const float pos[] = {
        float(x),   float(y+h),
        float(x+w), float(y+h),
        float(x),   float(y),
        float(x+w), float(y),
    };
    const float uv[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };
    status_t err;

    err = beforeDraw(texName, texMatrix, pos, uv, invert);
    if (err == NO_ERROR) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        err = afterDraw();
    }
    return err;
}

status_t Program::drawTriangles(GLuint texName, const float* texMatrix,
        const float* vertices, const float* texes, size_t count) const {
    ALOGV("Program::drawTriangles texName=%d", texName);

    status_t err;

    err = beforeDraw(texName, texMatrix, vertices, texes, false);
    if (err == NO_ERROR) {
        glDrawArrays(GL_TRIANGLES, 0, count);
        err = afterDraw();
    }
    return err;
}

status_t Program::beforeDraw(GLuint texName, const float* texMatrix,
        const float* vertices, const float* texes, bool invert) const {
    // Create an orthographic projection matrix based on viewport size.
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float screenToNdc[16] = {
        2.0f/float(vp[2]),  0.0f,               0.0f,   0.0f,
        0.0f,               -2.0f/float(vp[3]), 0.0f,   0.0f,
        0.0f,               0.0f,               1.0f,   0.0f,
        -1.0f,              1.0f,               0.0f,   1.0f,
    };
    if (invert) {
        screenToNdc[5] = -screenToNdc[5];
        screenToNdc[13] = -screenToNdc[13];
    }

    glUseProgram(mProgram);

    glVertexAttribPointer(maPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(maTextureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, texes);
    glEnableVertexAttribArray(maPositionLoc);
    glEnableVertexAttribArray(maTextureCoordLoc);

    glUniformMatrix4fv(muMVPMatrixLoc, 1, GL_FALSE, screenToNdc);
    glUniformMatrix4fv(muGLCMatrixLoc, 1, GL_FALSE, texMatrix);

    glActiveTexture(GL_TEXTURE0);

    switch (mProgramType) {
    case PROGRAM_EXTERNAL_TEXTURE:
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, texName);
        break;
    case PROGRAM_TEXTURE_2D:
        glBindTexture(GL_TEXTURE_2D, texName);
        break;
    default:
        ALOGE("unexpected program type %d", mProgramType);
        return UNKNOWN_ERROR;
    }

    glUniform1i(muTextureLoc, 0);

    GLenum glErr;
    if ((glErr = glGetError()) != GL_NO_ERROR) {
        ALOGE("GL error before draw: %#x", glErr);
        glDisableVertexAttribArray(maPositionLoc);
        glDisableVertexAttribArray(maTextureCoordLoc);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t Program::afterDraw() const {
    glDisableVertexAttribArray(maPositionLoc);
    glDisableVertexAttribArray(maTextureCoordLoc);

    GLenum glErr;
    if ((glErr = glGetError()) != GL_NO_ERROR) {
        ALOGE("GL error after draw: %#x", glErr);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}
