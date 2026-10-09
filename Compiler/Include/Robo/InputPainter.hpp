#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Input.hpp"

namespace Robo {

struct InputPaintResult {
    float advanceY = 0.0f;
    InputBounds bounds;
};

class InputPainter {
public:
    static InputPaintResult Paint(FrameBuffer& buffer,
                                  const FontEngine& fonts,
                                  Input& input,
                                  float originX,
                                  float originY,
                                  float containerWidth,
                                  float pixelScale = 1.0f);
};

}  // namespace Robo
