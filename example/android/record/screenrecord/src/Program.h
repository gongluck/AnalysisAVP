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

#ifndef SCREENRECORD_PROGRAM_H
#define SCREENRECORD_PROGRAM_H

#include <utils/Errors.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace android {

/*
 * Utility class for GLES rendering.
 *
 * Not thread-safe.
 */
class Program {
public:
    enum ProgramType { PROGRAM_UNKNOWN=0, PROGRAM_EXTERNAL_TEXTURE,
            PROGRAM_TEXTURE_2D };

    Program() :
        mProgramType(PROGRAM_UNKNOWN),
        mProgram(0),
        maPositionLoc(0),
        maTextureCoordLoc(0),
        muMVPMatrixLoc(0),
        muGLCMatrixLoc(0),
        muTextureLoc(0)
        {}
    ~Program() { release(); }

    // Initialize the program for use with the specified texture type.
    status_t setup(ProgramType type);

    // Release the program and associated resources.
    void release();

    // Blit the specified texture to { x, y, x+w, y+h }.  Inverts the
    // content if "invert" is set.
    status_t blit(GLuint texName, const float* texMatrix,
            int32_t x, int32_t y, int32_t w, int32_t h,
            bool invert = false) const;

    // Draw a number of triangles.
    status_t drawTriangles(GLuint texName, const float* texMatrix,
            const float* vertices, const float* texes, size_t count) const;

    static const float kIdentity[];

private:
    Program(const Program&);
    Program& operator=(const Program&);

    // Common code for draw functions.
    status_t beforeDraw(GLuint texName, const float* texMatrix,
            const float* vertices, const float* texes, bool invert) const;
    status_t afterDraw() const;

    // GLES 2 shader utilities.
    status_t createProgram(GLuint* outPgm, const char* vertexShader,
            const char* fragmentShader);
    static status_t compileShader(GLenum shaderType, const char* src,
            GLuint* outShader);
    static status_t linkShaderProgram(GLuint vs, GLuint fs, GLuint* outPgm);

    ProgramType mProgramType;
    GLuint mProgram;

    GLint maPositionLoc;
    GLint maTextureCoordLoc;
    GLint muMVPMatrixLoc;
    GLint muGLCMatrixLoc;
    GLint muTextureLoc;
};

}; // namespace android

#endif /*SCREENRECORD_PROGRAM_H*/
