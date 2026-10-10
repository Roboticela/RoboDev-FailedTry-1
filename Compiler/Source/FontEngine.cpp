#include "Robo/FontEngine.hpp"
#include "Robo/TextSelection.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "StbTrueType.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

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

const FontEngine::CachedGlyph& FontEngine::CachedGlyphFor(int faceIndex,
                                                             int glyph,
                                                             float fontSize,
                                                             float shiftX,
                                                             float shiftY) const {
    constexpr int OVERSAMPLE = 2;
    const int subX = std::max(0, std::min(3, static_cast<int>(shiftX * 4.0f)));
    const int subY = std::max(0, std::min(3, static_cast<int>(shiftY * 4.0f)));
    const int sizeQ = std::max(1, static_cast<int>(std::round(fontSize * 4.0f)));
    const std::uint64_t key = (static_cast<std::uint64_t>(faceIndex) & 0xF)
                              | ((static_cast<std::uint64_t>(glyph) & 0xFFFFF) << 4)
                              | ((static_cast<std::uint64_t>(sizeQ) & 0xFFFF) << 24)
                              | (static_cast<std::uint64_t>(subX) << 40)
                              | (static_cast<std::uint64_t>(subY) << 44);
    const auto found = glyphCache_.find(key);
    if (found != glyphCache_.end()) {
        return found->second;
    }

    CachedGlyph cached;
    const FaceData& face = *faces_[faceIndex];
    const float snappedSize = static_cast<float>(sizeQ) / 4.0f;
    const float scale = stbtt_ScaleForPixelHeight(&face.info, snappedSize);
    const float snappedShiftX = static_cast<float>(subX) * 0.25f;
    const float snappedShiftY = static_cast<float>(subY) * 0.25f;
    const float renderScale = scale * static_cast<float>(OVERSAMPLE);
    int ix0 = 0;
    int iy0 = 0;
    int ix1 = 0;
    int iy1 = 0;
    stbtt_GetGlyphBitmapBoxSubpixel(&face.info,
                                    glyph,
                                    renderScale,
                                    renderScale,
                                    snappedShiftX * static_cast<float>(OVERSAMPLE),
                                    snappedShiftY * static_cast<float>(OVERSAMPLE),
                                    &ix0,
                                    &iy0,
                                    &ix1,
                                    &iy1);
    const int glyphWidth = ix1 - ix0;
    const int glyphHeight = iy1 - iy0;
    if (glyphWidth <= 0 || glyphHeight <= 0) {
        return glyphCache_.emplace(key, std::move(cached)).first->second;
    }

    std::vector<unsigned char> bitmap(static_cast<std::size_t>(glyphWidth * glyphHeight));
    stbtt_MakeGlyphBitmapSubpixel(&face.info,
                                  bitmap.data(),
                                  glyphWidth,
                                  glyphHeight,
                                  glyphWidth,
                                  renderScale,
                                  renderScale,
                                  snappedShiftX * static_cast<float>(OVERSAMPLE),
                                  snappedShiftY * static_cast<float>(OVERSAMPLE),
                                  glyph);

    auto FloorDiv = [](int value, int divisor) {
        return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
    };
    const int outW = FloorDiv(ix1 - 1, OVERSAMPLE) - FloorDiv(ix0, OVERSAMPLE) + 1;
    const int outH = FloorDiv(iy1 - 1, OVERSAMPLE) - FloorDiv(iy0, OVERSAMPLE) + 1;
    cached.width = std::max(0, outW);
    cached.height = std::max(0, outH);
    cached.xOff = FloorDiv(ix0, OVERSAMPLE);
    cached.yOff = FloorDiv(iy0, OVERSAMPLE);
    cached.coverage.assign(static_cast<std::size_t>(cached.width * cached.height), 0);
    const int originSampleX = FloorDiv(ix0, OVERSAMPLE) * OVERSAMPLE;
    const int originSampleY = FloorDiv(iy0, OVERSAMPLE) * OVERSAMPLE;
    const float sampleCount = static_cast<float>(OVERSAMPLE * OVERSAMPLE);
    for (int row = 0; row < cached.height; ++row) {
        for (int col = 0; col < cached.width; ++col) {
            unsigned int sum = 0;
            const int sampleX0 = originSampleX + col * OVERSAMPLE;
            const int sampleY0 = originSampleY + row * OVERSAMPLE;
            for (int sampleY = 0; sampleY < OVERSAMPLE; ++sampleY) {
                for (int sampleX = 0; sampleX < OVERSAMPLE; ++sampleX) {
                    const int sourceX = sampleX0 + sampleX - ix0;
                    const int sourceY = sampleY0 + sampleY - iy0;
                    if (sourceX < 0 || sourceY < 0 || sourceX >= glyphWidth || sourceY >= glyphHeight) {
                        continue;
                    }
                    sum += bitmap[static_cast<std::size_t>(sourceY * glyphWidth + sourceX)];
                }
            }
            cached.coverage[static_cast<std::size_t>(row * cached.width + col)] =
                static_cast<unsigned char>(std::min(255, static_cast<int>(sum / sampleCount + 0.5f)));
        }
    }
    return glyphCache_.emplace(key, std::move(cached)).first->second;
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
    const float ascent = GetAscent(fontSize, faceId);
    const float glyphHeight = std::max(1.0f, ascent + GetDescent(fontSize, faceId));
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

        int advance = 0;
        int lsb = 0;
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &lsb);
        const float glyphWidth = std::max(0.0f, scale * static_cast<float>(advance));
        if (TextSelection::IsOpen()) {
            const int index = TextSelection::AddCodepoint(static_cast<char32_t>(codepoint),
                                                          penX,
                                                          baselineY - ascent,
                                                          std::max(1.0f, glyphWidth),
                                                          glyphHeight);
            if (TextSelection::Covers(index)) {
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                static_cast<int>(std::floor(baselineY - ascent)),
                                std::max(1, static_cast<int>(std::ceil(glyphWidth))),
                                std::max(1, static_cast<int>(std::ceil(glyphHeight))),
                                Color::FromRgba(0, 122, 255, 255),
                                0.28f);
            }
            if (TextSelection::IsCaretBefore(index) || TextSelection::IsCaretAfter(index)) {
                const float caretX = TextSelection::IsCaretAfter(index) ? penX + glyphWidth : penX;
                buffer.FillRect(static_cast<int>(std::floor(caretX)),
                                static_cast<int>(std::floor(baselineY - ascent)),
                                std::max(1, static_cast<int>(std::round(std::max(1.0f, scale)))),
                                std::max(1, static_cast<int>(std::ceil(glyphHeight))),
                                Color::FromHex("#007AFF"),
                                1.0f);
            }
        }

        const float shiftX = penX - std::floor(penX);
        const float shiftY = baselineY - std::floor(baselineY);
        const bool onScreen = baselineY > -fontSize * 2.0f && baselineY < static_cast<float>(buffer.GetHeight()) + fontSize * 2.0f;
        if (onScreen) {
            const CachedGlyph& cached = CachedGlyphFor(FaceIndex(faceId), glyph, fontSize, shiftX, shiftY);
            const int destX = static_cast<int>(std::floor(penX)) + cached.xOff;
            const int destY = static_cast<int>(std::floor(baselineY)) + cached.yOff;
            for (int row = 0; row < cached.height; ++row) {
                const int py = destY + row;
                if (py < 0 || py >= buffer.GetHeight()) {
                    continue;
                }
                for (int col = 0; col < cached.width; ++col) {
                    const unsigned char coverage = cached.coverage[static_cast<std::size_t>(row * cached.width + col)];
                    if (coverage == 0) {
                        continue;
                    }
                    buffer.BlendPixel(destX + col, py, color, (coverage / 255.0f) * opacity);
                }
            }
        }

        penX += glyphWidth;
        drewGlyph = true;
        previousGlyph = glyph;
    }
}

}  // namespace Robo
