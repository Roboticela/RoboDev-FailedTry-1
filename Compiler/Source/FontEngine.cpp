#include "Robo/FontEngine.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "StbTrueType.h"

#include <cmath>
#include <cstring>

namespace Robo {

namespace {

int DecodeUtf8(const char*& cursor, const char* end) {
    if (cursor >= end) {
        return -1;
    }
    const unsigned char lead = static_cast<unsigned char>(*cursor++);
    if (lead < 0x80) {
        return lead;
    }
    if ((lead >> 5) == 0x6 && cursor < end) {
        const unsigned char b1 = static_cast<unsigned char>(*cursor++);
        return ((lead & 0x1F) << 6) | (b1 & 0x3F);
    }
    if ((lead >> 4) == 0xE && cursor + 1 < end) {
        const unsigned char b1 = static_cast<unsigned char>(*cursor++);
        const unsigned char b2 = static_cast<unsigned char>(*cursor++);
        return ((lead & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
    }
    if ((lead >> 3) == 0x1E && cursor + 2 < end) {
        const unsigned char b1 = static_cast<unsigned char>(*cursor++);
        const unsigned char b2 = static_cast<unsigned char>(*cursor++);
        const unsigned char b3 = static_cast<unsigned char>(*cursor++);
        return ((lead & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
    }
    return -1;
}

int FaceIndex(FontFaceId faceId) {
    return static_cast<int>(faceId);
}

bool IsSkippableCodepoint(int codepoint) {
    return codepoint < 32 || codepoint == 0x7F || codepoint == 0xFFFD;
}

}  // namespace

struct FontEngine::FaceData {
    std::vector<std::uint8_t> bytes;
    stbtt_fontinfo info{};
    bool ready = false;
};

FontEngine::FontEngine() {
    for (int i = 0; i < 4; ++i) {
        faces_[i] = std::make_unique<FaceData>();
    }
}

FontEngine::~FontEngine() = default;

bool FontEngine::LoadFace(FontFaceId faceId, const std::vector<std::uint8_t>& ttfBytes) {
    FaceData& face = *faces_[FaceIndex(faceId)];
    face.bytes = ttfBytes;
    face.ready = false;
    if (face.bytes.empty()) {
        return false;
    }
    if (!stbtt_InitFont(&face.info, face.bytes.data(), stbtt_GetFontOffsetForIndex(face.bytes.data(), 0))) {
        return false;
    }
    face.ready = true;
    return true;
}

bool FontEngine::IsReady() const {
    return faces_[FaceIndex(FontFaceId::Regular)]->ready;
}

FontFaceId FontEngine::ResolveFace(FontWeight weight, FontStyleKind style) const {
    if (style == FontStyleKind::Italic && faces_[FaceIndex(FontFaceId::Italic)]->ready) {
        return FontFaceId::Italic;
    }
    if (weight == FontWeight::Bold || weight == FontWeight::SemiBold) {
        if (faces_[FaceIndex(FontFaceId::SemiBold)]->ready) {
            return FontFaceId::SemiBold;
        }
    }
    if (weight == FontWeight::Medium && faces_[FaceIndex(FontFaceId::Medium)]->ready) {
        return FontFaceId::Medium;
    }
    return FontFaceId::Regular;
}

float FontEngine::MeasureWidth(const std::string& utf8, float fontSize, FontFaceId faceId, float letterSpacing) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready || fontSize <= 0.0f) {
        return 0.0f;
    }

    const float scale = stbtt_ScaleForPixelHeight(&face.info, fontSize);
    const char* cursor = utf8.data();
    const char* end = utf8.data() + utf8.size();
    float width = 0.0f;
    int previousGlyph = -1;
    int glyphCount = 0;

    while (cursor < end) {
        const int codepoint = DecodeUtf8(cursor, end);
        if (codepoint < 0) {
            break;
        }
        if (IsSkippableCodepoint(codepoint)) {
            previousGlyph = -1;
            continue;
        }

        const int glyph = stbtt_FindGlyphIndex(&face.info, codepoint);
        if (glyph == 0) {
            previousGlyph = -1;
            continue;
        }

        int advance = 0;
        int leftSideBearing = 0;
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &leftSideBearing);
        if (previousGlyph >= 0) {
            width += scale * stbtt_GetGlyphKernAdvance(&face.info, previousGlyph, glyph);
        }
        width += scale * advance;
        previousGlyph = glyph;
        ++glyphCount;
    }

    if (glyphCount > 1) {
        width += letterSpacing * static_cast<float>(glyphCount - 1);
    }
    return width;
}

float FontEngine::GetAscent(float fontSize, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready) {
        return fontSize * 0.8f;
    }
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
    return stbtt_ScaleForPixelHeight(&face.info, fontSize) * ascent;
}

float FontEngine::GetDescent(float fontSize, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready) {
        return fontSize * 0.2f;
    }
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
    return stbtt_ScaleForPixelHeight(&face.info, fontSize) * static_cast<float>(-descent);
}

float FontEngine::GetLineGap(float fontSize, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready) {
        return 0.0f;
    }
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
    return stbtt_ScaleForPixelHeight(&face.info, fontSize) * lineGap;
}

void FontEngine::DrawString(FrameBuffer& buffer,
                            float x,
                            float baselineY,
                            const std::string& utf8,
                            float fontSize,
                            FontFaceId faceId,
                            const Color& color,
                            float opacity,
                            float letterSpacing) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready || utf8.empty() || fontSize <= 0.0f) {
        return;
    }

    const float scale = stbtt_ScaleForPixelHeight(&face.info, fontSize);
    const char* cursor = utf8.data();
    const char* end = utf8.data() + utf8.size();
    float penX = x;
    int previousGlyph = -1;
    bool drewGlyph = false;

    while (cursor < end) {
        const int codepoint = DecodeUtf8(cursor, end);
        if (codepoint < 0) {
            break;
        }
        if (IsSkippableCodepoint(codepoint)) {
            previousGlyph = -1;
            continue;
        }

        const int glyph = stbtt_FindGlyphIndex(&face.info, codepoint);
        if (glyph == 0) {
            previousGlyph = -1;
            continue;
        }

        if (drewGlyph) {
            penX += letterSpacing;
        }
        if (previousGlyph >= 0) {
            penX += scale * stbtt_GetGlyphKernAdvance(&face.info, previousGlyph, glyph);
        }

        const float shiftX = penX - std::floor(penX);
        const float shiftY = baselineY - std::floor(baselineY);
        int xoff = 0;
        int yoff = 0;
        int glyphWidth = 0;
        int glyphHeight = 0;
        unsigned char* bitmap = stbtt_GetGlyphBitmapSubpixel(
            &face.info, scale, scale, shiftX, shiftY, glyph, &glyphWidth, &glyphHeight, &xoff, &yoff);

        if (bitmap && glyphWidth > 0 && glyphHeight > 0) {
            const int destX = static_cast<int>(std::floor(penX)) + xoff;
            const int destY = static_cast<int>(std::floor(baselineY)) + yoff;
            for (int row = 0; row < glyphHeight; ++row) {
                for (int col = 0; col < glyphWidth; ++col) {
                    const unsigned char coverage = bitmap[row * glyphWidth + col];
                    if (coverage == 0) {
                        continue;
                    }
                    // Slight contrast curve for cleaner edges on light backgrounds.
                    float normalized = coverage / 255.0f;
                    normalized = normalized * (normalized * 0.35f + 0.65f);
                    buffer.BlendPixel(destX + col, destY + row, color, normalized * opacity);
                }
            }
            stbtt_FreeBitmap(bitmap, nullptr);
        } else if (bitmap) {
            stbtt_FreeBitmap(bitmap, nullptr);
        }

        int advance = 0;
        int lsb = 0;
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &lsb);
        penX += scale * static_cast<float>(advance);
        drewGlyph = true;
        previousGlyph = glyph;
    }
}

}  // namespace Robo
