#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Text.hpp"

namespace Robo {

class TextPainter {
public:
    static float ResolveWidthValue(const std::string& value, float referenceWidth);
    static float Paint(FrameBuffer& buffer,
                       const FontEngine& fonts,
                       const Text& text,
                       float originX,
                       float originY,
                       float containerWidth,
                       float pixelScale = 1.0f);
};

}  // namespace Robo
