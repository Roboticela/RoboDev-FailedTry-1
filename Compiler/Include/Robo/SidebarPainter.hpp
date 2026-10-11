#pragma once

#include "FrameBuffer.hpp"
#include "Sidebar.hpp"

namespace Robo {

struct SidebarMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

class SidebarPainter {
public:
    static SidebarMeasureResult Measure(float availableHeight, float pixelScale);
    static void Paint(FrameBuffer& buffer, float originX, float originY, float height, float pixelScale);
};

}  // namespace Robo
