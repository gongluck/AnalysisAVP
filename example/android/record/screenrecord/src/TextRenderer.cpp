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

#include "TextRenderer.h"

#include <assert.h>
#include <malloc.h>
#include <string.h>

namespace android {
#include "FontBitmap.h"
};

using namespace android;

const char TextRenderer::kWhitespace[] = " \t\n\r";

bool TextRenderer::mInitialized = false;
uint32_t TextRenderer::mXOffset[FontBitmap::numGlyphs];

void TextRenderer::initOnce() {
    if (!mInitialized) {
        initXOffset();
        mInitialized = true;
    }
}

void TextRenderer::initXOffset() {
    // Generate a table of X offsets.  They start at zero and reset whenever
    // we move down a line (i.e. the Y offset changes).  The offset increases
    // by one pixel more than the width because the generator left a gap to
    // avoid reading pixels from adjacent glyphs in the texture filter.
    uint16_t offset = 0;
    uint16_t prevYOffset = (int16_t) -1;
    for (unsigned int i = 0; i < FontBitmap::numGlyphs; i++) {
        if (prevYOffset != FontBitmap::yoffset[i]) {
            prevYOffset = FontBitmap::yoffset[i];
            offset = 0;
        }
        mXOffset[i] = offset;
        offset += FontBitmap::glyphWidth[i] + 1;
    }
}

static bool isPowerOfTwo(uint32_t val) {
    // a/k/a "is exactly one bit set"; note returns true for 0
    return (val & (val -1)) == 0;
}

static uint32_t powerOfTwoCeil(uint32_t val) {
    // drop it, smear the bits across, pop it
    val--;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val++;

    return val;
}

float TextRenderer::getGlyphHeight() const {
    return FontBitmap::maxGlyphHeight;
}

status_t TextRenderer::loadIntoTexture() {
    ALOGV("Font::loadIntoTexture");

    glGenTextures(1, &mTextureName);
    if (mTextureName == 0) {
        ALOGE("glGenTextures failed: %#x", glGetError());
        return UNKNOWN_ERROR;
    }
    glBindTexture(GL_TEXTURE_2D, mTextureName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // The pixel data is stored as combined color+alpha, 8 bits per pixel.
    // It's guaranteed to be a power-of-two wide, but we cut off the height
    // where the data ends.  We want to expand it to a power-of-two bitmap
    // with ARGB data and hand that to glTexImage2D.

    if (!isPowerOfTwo(FontBitmap::width)) {
        ALOGE("npot glyph bitmap width %u", FontBitmap::width);
        return UNKNOWN_ERROR;
    }

    uint32_t potHeight = powerOfTwoCeil(FontBitmap::height);
    uint8_t* rgbaPixels = new uint8_t[FontBitmap::width * potHeight * 4];
    memset(rgbaPixels, 0, FontBitmap::width * potHeight * 4);
    uint8_t* pix = rgbaPixels;

    for (unsigned int i = 0; i < FontBitmap::width * FontBitmap::height; i++) {
        uint8_t alpha, color;
        if ((FontBitmap::pixels[i] & 1) == 0) {
            // black pixel with varying alpha
            color = 0x00;
            alpha = FontBitmap::pixels[i] & ~1;
        } else {
            // opaque grey pixel
            color = FontBitmap::pixels[i] & ~1;
            alpha = 0xff;
        }
        *pix++ = color;
        *pix++ = color;
        *pix++ = color;
        *pix++ = alpha;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FontBitmap::width, potHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    delete[] rgbaPixels;
    GLint glErr = glGetError();
    if (glErr != 0) {
        ALOGE("glTexImage2D failed: %#x", glErr);
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

void TextRenderer::setProportionalScale(float linesPerScreen) {
    if (mScreenWidth == 0 || mScreenHeight == 0) {
        ALOGW("setFontScale: can't set scale for width=%d height=%d",
                mScreenWidth, mScreenHeight);
        return;
    }
    float tallest = mScreenWidth > mScreenHeight ? mScreenWidth : mScreenHeight;
    setScale(tallest / (linesPerScreen * getGlyphHeight()));
}

float TextRenderer::computeScaledStringWidth(const String8& str8) const {
    // String8.length() isn't documented, but I'm assuming it will return
    // the number of characters rather than the number of bytes.  Since
    // we can only display ASCII we want to ignore anything else, so we
    // just convert to char* -- but String8 doesn't document what it does
    // with values outside 0-255.  So just convert to char* and use strlen()
    // to see what we get.
    const char* str = str8.string();
    return computeScaledStringWidth(str, strlen(str));
}

size_t TextRenderer::glyphIndex(char ch) const {
    size_t chi = ch - FontBitmap::firstGlyphChar;
    if (chi >= FontBitmap::numGlyphs) {
        chi = '?' - FontBitmap::firstGlyphChar;
    }
    assert(chi < FontBitmap::numGlyphs);
    return chi;
}

float TextRenderer::computeScaledStringWidth(const char* str,
        size_t len) const {
    float width = 0.0f;
    for (size_t i = 0; i < len; i++) {
        size_t chi = glyphIndex(str[i]);
        float glyphWidth = FontBitmap::glyphWidth[chi];
        width += (glyphWidth - 1 - FontBitmap::outlineWidth) * mScale;
    }

    return width;
}

void TextRenderer::drawString(const Program& program, const float* texMatrix,
        float x, float y, const String8& str8) const {
    ALOGV("drawString %.3f,%.3f '%s' (scale=%.3f)", x, y, str8.string(),mScale);
    initOnce();

    // We want to draw the entire string with a single GLES call.  We
    // generate two arrays, one with screen coordinates, one with texture
    // coordinates.  Need two triangles per character.
    const char* str = str8.string();
    size_t len = strlen(str);       // again, unsure about String8 handling

    const size_t quadCoords =
            2 /*triangles*/ * 3 /*vertex/tri*/ * 2 /*coord/vertex*/;
    float vertices[len * quadCoords];
    float texes[len * quadCoords];

    float fullTexWidth = FontBitmap::width;
    float fullTexHeight = powerOfTwoCeil(FontBitmap::height);
    for (size_t i = 0; i < len; i++) {
        size_t chi = glyphIndex(str[i]);
        float glyphWidth = FontBitmap::glyphWidth[chi];
        float glyphHeight = FontBitmap::maxGlyphHeight;

        float vertLeft = x;
        float vertRight = x + glyphWidth * mScale;
        float vertTop = y;
        float vertBottom = y + glyphHeight * mScale;

        // Lowest-numbered glyph is in top-left of bitmap, which puts it at
        // the bottom-left in texture coordinates.
        float texLeft = mXOffset[chi] / fullTexWidth;
        float texRight = (mXOffset[chi] + glyphWidth) / fullTexWidth;
        float texTop = FontBitmap::yoffset[chi] / fullTexHeight;
        float texBottom = (FontBitmap::yoffset[chi] + glyphHeight) /
                fullTexHeight;

        size_t off = i * quadCoords;
        vertices[off +  0] = vertLeft;
        vertices[off +  1] = vertBottom;
        vertices[off +  2] = vertRight;
        vertices[off +  3] = vertBottom;
        vertices[off +  4] = vertLeft;
        vertices[off +  5] = vertTop;
        vertices[off +  6] = vertLeft;
        vertices[off +  7] = vertTop;
        vertices[off +  8] = vertRight;
        vertices[off +  9] = vertBottom;
        vertices[off + 10] = vertRight;
        vertices[off + 11] = vertTop;
        texes[off +  0] = texLeft;
        texes[off +  1] = texBottom;
        texes[off +  2] = texRight;
        texes[off +  3] = texBottom;
        texes[off +  4] = texLeft;
        texes[off +  5] = texTop;
        texes[off +  6] = texLeft;
        texes[off +  7] = texTop;
        texes[off +  8] = texRight;
        texes[off +  9] = texBottom;
        texes[off + 10] = texRight;
        texes[off + 11] = texTop;

        // We added 1-pixel padding in the texture, so we want to advance by
        // one less.  Also, each glyph is surrounded by a black outline, which
        // we want to merge.
        x += (glyphWidth - 1 - FontBitmap::outlineWidth) * mScale;
    }

    program.drawTriangles(mTextureName, texMatrix, vertices, texes,
            len * quadCoords / 2);
}

float TextRenderer::drawWrappedString(const Program& texRender,
        float xpos, float ypos, const String8& str) {
    ALOGV("drawWrappedString %.3f,%.3f '%s'", xpos, ypos, str.string());
    initOnce();

    if (mScreenWidth == 0 || mScreenHeight == 0) {
        ALOGW("drawWrappedString: can't wrap with width=%d height=%d",
                mScreenWidth, mScreenHeight);
        return ypos;
    }

    const float indentWidth = mIndentMult * getScale();
    if (xpos < mBorderWidth) {
        xpos = mBorderWidth;
    }
    if (ypos < mBorderWidth) {
        ypos = mBorderWidth;
    }

    const size_t maxWidth = (mScreenWidth - mBorderWidth) - xpos;
    if (maxWidth < 1) {
        ALOGE("Unable to render text: xpos=%.3f border=%.3f width=%u",
                xpos, mBorderWidth, mScreenWidth);
        return ypos;
    }
    float stringWidth = computeScaledStringWidth(str);
    if (stringWidth <= maxWidth) {
        // Trivial case.
        drawString(texRender, Program::kIdentity, xpos, ypos, str);
        ypos += getScaledGlyphHeight();
    } else {
        // We need to break the string into pieces, ideally at whitespace
        // boundaries.
        char* mangle = strdup(str.string());
        char* start = mangle;
        while (start != NULL) {
            float xposAdj = (start == mangle) ? xpos : xpos + indentWidth;
            char* brk = breakString(start,
                    (float) (mScreenWidth - mBorderWidth - xposAdj));
            if (brk == NULL) {
                // draw full string
                drawString(texRender, Program::kIdentity, xposAdj, ypos,
                        String8(start));
                start = NULL;
            } else {
                // draw partial string
                char ch = *brk;
                *brk = '\0';
                drawString(texRender, Program::kIdentity, xposAdj, ypos,
                        String8(start));
                *brk = ch;
                start = brk;
                if (strchr(kWhitespace, ch) != NULL) {
                    // if we broke on whitespace, skip past it
                    start++;
                }
            }
            ypos += getScaledGlyphHeight();
        }
        free(mangle);
    }

    return ypos;
}

char* TextRenderer::breakString(const char* str, float maxWidth) const {
    // Ideally we'd do clever things like binary search.  Not bothering.
    ALOGV("breakString '%s' %.3f", str, maxWidth);

    size_t len = strlen(str);
    if (len == 0) {
        // Caller should detect this and not advance ypos.
        return NULL;
    }

    float stringWidth = computeScaledStringWidth(str, len);
    if (stringWidth <= maxWidth) {
        return NULL;        // trivial -- use full string
    }

    // Find the longest string that will fit.
    size_t goodPos = 0;
    for (size_t i = 0; i < len; i++) {
        stringWidth = computeScaledStringWidth(str, i);
        if (stringWidth < maxWidth) {
            goodPos = i;
        } else {
            break;  // too big
        }
    }
    if (goodPos == 0) {
        // space is too small to hold any glyph; output a single char
        ALOGW("Couldn't find a nonzero prefix that fit from '%s'", str);
        goodPos = 1;
    }

    // Scan back for whitespace.  If we can't find any we'll just have
    // an ugly mid-word break.
    for (size_t i = goodPos; i > 0; i--) {
        if (strchr(kWhitespace, str[i]) != NULL) {
            goodPos = i;
            break;
        }
    }

    ALOGV("goodPos=%zu for str='%s'", goodPos, str);
    return const_cast<char*>(str + goodPos);
}
