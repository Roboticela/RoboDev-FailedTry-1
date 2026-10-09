#pragma once

#include "Button.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

class ButtonPainter {
public:
    static float Paint(FrameBuffer& buffer,
                       const FontEngine& fonts,
                       const Button& button,
                       float originX,
                       float originY,
                       float containerWidth);
};

}  // namespace Robo
