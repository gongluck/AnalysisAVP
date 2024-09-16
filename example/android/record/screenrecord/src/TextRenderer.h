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

#ifndef SCREENRECORD_TEXT_RENDER_H
#define SCREENRECORD_TEXT_RENDER_H

#include "Program.h"

#include <utils/String8.h>
#include <utils/Errors.h>

#include <GLES2/gl2.h>


namespace android {

/*
 * Simple font representation.
 *
 * Not thread-safe.
 */
class TextRenderer {
public:
    TextRenderer() :
        mTextureName(0),
        mScale(1.0f),
        mBorderWidth(10.0f),
        mIndentMult(30.0f),
        mScreenWidth(0),
        mScreenHeight(0)
        {}
    ~TextRenderer() {}

    // Load the glyph bitmap into a 2D texture in the current context.
    status_t loadIntoTexture();

    // Set the screen dimensions, used for scaling and line wrap.
    void setScreenSize(uint32_t width, uint32_t height) {
        mScreenWidth = width;
        mScreenHeight = height;
    }

    // Get/set the font scaling.
    float getScale() const { return mScale; }
    void setScale(float scale) { mScale = scale; }

    // Set the font scaling based on the desired number of lines per screen.
    // The display's tallest axis is used, so if the device is in landscape
    // the screen will fit fewer lines.
    void setProportionalScale(float linesPerScreen);

    // Render the text string at the specified coordinates.  Pass in the
    // upper-left corner in non-GL-flipped coordinates, i.e. to print text
    // at the top left of the screen use (0,0).
    //
    // Set blend func (1, 1-srcAlpha) before calling if drawing onto
    // something other than black.
    void drawString(const Program& program, const float* texMatrix,
            float x, float y, const String8& str) const;

    // Draw a string, possibly wrapping it at the screen boundary.  Top-left
    // is at (0,0).
    //
    // Returns the updated Y position.
    float drawWrappedString(const Program& texRender, float xpos, float ypos,
            const String8& str);

    // Returns the name of the texture the font was loaded into.
    GLuint getTextureName() const { return mTextureName; }

private:
    TextRenderer(const TextRenderer&);
    TextRenderer& operator=(const TextRenderer&);

    // Perform one-time initialization.
    static void initOnce();

    // Populate the mXOffset array.
    static void initXOffset();

    // Find a good place to break the string.  Returns NULL if the entire
    // string will fit.
    char* breakString(const char* str, float maxWidth) const;

    // Computes the width of the string, in pixels.
    float computeScaledStringWidth(const String8& str8) const;

    // Computes the width of first N characters in the string.
    float computeScaledStringWidth(const char* str, size_t len) const;

    // Returns the font's glyph height.  This is the full pixel height of the
    // tallest glyph, both above and below the baseline, NOT adjusted by the
    // current scale factor.
    float getGlyphHeight() const;

    // Like getGlyphHeight(), but result is scaled.
    float getScaledGlyphHeight() const { return getGlyphHeight() * mScale; }

    // Convert an ASCII character to a glyph index.  Returns the glyph for
    // '?' if we have no glyph for the specified character.
    size_t glyphIndex(char ch) const;

    GLuint mTextureName;
    float mScale;

    // Number of pixels preserved at the left/right edges of the screen by
    // drawWrappedString().  Not scaled.
    float mBorderWidth;

    // Distance to indent a broken line.  Used by drawWrappedString().
    // Value will be adjusted by the current scale factor.
    float mIndentMult;

    // Screen dimensions.
    uint32_t mScreenWidth;
    uint32_t mScreenHeight;

    // Static font info.
    static bool mInitialized;
    static uint32_t mXOffset[];

    static const char kWhitespace[];
};

}; // namespace android

#endif /*SCREENRECORD_TEXT_RENDER_H*/
