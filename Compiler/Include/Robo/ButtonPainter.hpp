#pragma once

#include "Button.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

struct ButtonPaintResult {
    float advanceY = 0.0f;
    ButtonBounds bounds;
};

class ButtonPainter {
public:
    static ButtonPaintResult Paint(FrameBuffer& buffer,
                                   const FontEngine& fonts,
                                   Button& button,
                                   float originX,
                                   float originY,
                                   float containerWidth);
};

}  // namespace Robo
