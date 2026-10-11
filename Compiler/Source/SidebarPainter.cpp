#include "Robo/SidebarPainter.hpp"

#include "Robo/Foundation.hpp"

#include <algorithm>

namespace Robo {

SidebarMeasureResult SidebarPainter::Measure(float availableHeight, float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    SidebarMeasureResult result;
    result.width = SIDEBAR_WIDTH * scale;
    result.height = std::max(0.0f, availableHeight);
    return result;
}

void SidebarPainter::Paint(FrameBuffer& buffer, float originX, float originY, float height, float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const float width = SIDEBAR_WIDTH * scale;
    const int paintHeight = std::max(1, static_cast<int>(std::ceil(height)));
    const Color background = ResolveColor(Color::FromHex("#F5F5F7"), "color.bg.secondary");
    buffer.FillRect(static_cast<int>(std::floor(originX)),
                    static_cast<int>(std::floor(originY)),
                    std::max(1, static_cast<int>(std::ceil(width))),
                    paintHeight,
                    background,
                    1.0f);
}

}  // namespace Robo
