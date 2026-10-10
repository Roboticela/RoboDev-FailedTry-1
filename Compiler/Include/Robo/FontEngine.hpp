#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "FrameBuffer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Robo {

enum class FontFaceId {
    Regular,
    Medium,
    SemiBold,
    Italic
};

class FontEngine {
public:
    FontEngine();
    ~FontEngine();

    bool LoadFace(FontFaceId faceId, const std::vector<std::uint8_t>& ttfBytes);
    bool IsReady() const;

    FontFaceId ResolveFace(FontWeight weight, FontStyleKind style) const;
    float MeasureWidth(const std::string& utf8, float fontSize, FontFaceId faceId, float letterSpacing = 0.0f) const;
    float GetAscent(float fontSize, FontFaceId faceId) const;
    float GetDescent(float fontSize, FontFaceId faceId) const;
    float GetLineGap(float fontSize, FontFaceId faceId) const;

    void DrawString(FrameBuffer& buffer,
                    float x,
                    float baselineY,
                    const std::string& utf8,
                    float fontSize,
                    FontFaceId faceId,
                    const Color& color,
                    float opacity,
                    float letterSpacing) const;

private:
    struct FaceData;
    struct CachedGlyph {
        int width = 0;
        int height = 0;
        int xOff = 0;
        int yOff = 0;
        std::vector<std::uint8_t> coverage;
    };

    const CachedGlyph& CachedGlyphFor(int faceIndex, int glyph, float fontSize, float shiftX, float shiftY) const;

    std::unique_ptr<FaceData> faces_[4];
    mutable std::unordered_map<std::uint64_t, CachedGlyph> glyphCache_;
};

}  // namespace Robo
