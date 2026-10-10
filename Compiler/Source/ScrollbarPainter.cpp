#include "Robo/ScrollbarPainter.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

void FillRoundedRect(FrameBuffer& buffer,
                     float x,
                     float y,
                     float width,
                     float height,
                     float radius,
                     const Color& color,
                     float opacity) {
    if (width <= 0.0f || height <= 0.0f || color.alpha == 0) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min({radius, width * 0.5f, height * 0.5f}));
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = static_cast<int>(std::ceil(x + width));
    const int y1 = static_cast<int>(std::ceil(y + height));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;
    const float radiusSq = cappedRadius * cappedRadius;

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;
            if (fx < x || fx > x + width || fy < y || fy > y + height) {
                continue;
            }

            bool inside = true;
            if (cappedRadius > 0.0f) {
                float cx = fx;
                float cy = fy;
                if (fx < left && fy < top) {
                    cx = left;
                    cy = top;
                } else if (fx > right && fy < top) {
                    cx = right;
                    cy = top;
                } else if (fx < left && fy > bottom) {
                    cx = left;
                    cy = bottom;
                } else if (fx > right && fy > bottom) {
                    cx = right;
                    cy = bottom;
                }

                if (cx != fx || cy != fy) {
                    const float dx = fx - cx;
                    const float dy = fy - cy;
                    inside = (dx * dx + dy * dy) <= radiusSq;
                }
            }

            if (inside) {
                buffer.BlendPixel(px, py, color, opacity);
            }
        }
    }
}

}  // namespace

ScrollbarMeasureResult ScrollbarPainter::Measure(const Scrollbar& scrollbar,
                                                 float containerWidth,
                                                 float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const ScrollbarStyle& style = scrollbar.GetStyle();

    const float thickness = static_cast<float>(style.thickness) * scale;
    float length = static_cast<float>(style.length) * scale;
    if (style.orientation == ScrollbarOrientation::Horizontal && containerWidth > 0.0f) {
        length = std::min(length, containerWidth);
    }
    length = std::max(0.0f, length);

    ScrollbarMeasureResult result;
    if (style.orientation == ScrollbarOrientation::Vertical) {
        result.width = thickness;
        result.height = length;
    } else {
        result.width = length;
        result.height = thickness;
    }
    result.advanceY = result.height + 16.0f * scale;
    return result;
}

ScrollbarPaintResult ScrollbarPainter::Paint(FrameBuffer& buffer,
                                             Scrollbar& scrollbar,
                                             float originX,
                                             float originY,
                                             float containerWidth,
                                             float pixelScale,
                                             float forcedWidth,
                                             float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    const ScrollbarStyle& style = scrollbar.GetStyle();

    const ScrollbarMeasureResult natural = Measure(scrollbar, containerWidth, scale);

    const float trackWidth = forcedWidth > 0.0f ? forcedWidth : natural.width;
    const float trackHeight = forcedHeight > 0.0f ? forcedHeight : natural.height;

    const float trackX = originX;
    const float trackY = originY;
    const float opacity = scrollbar.ResolveOpacity();

    ScrollbarBounds bounds;
    bounds.x = trackX;
    bounds.y = trackY;
    bounds.width = std::max(0.0f, trackWidth);
    bounds.height = std::max(0.0f, trackHeight);

    scrollbar.SetPixelScale(scale);
    scrollbar.SetBounds(bounds);

    const float radius = static_cast<float>(style.cornerRadius) * scale;

    const Color trackColor = scrollbar.ResolveTrackColor();
    if (trackColor.alpha > 0 && bounds.width > 0.0f && bounds.height > 0.0f) {
        FillRoundedRect(buffer, bounds.x, bounds.y, bounds.width, bounds.height, radius, trackColor, opacity);
    }

    const ScrollbarBounds thumb = scrollbar.ResolveThumbBounds();
    const Color thumbColor = scrollbar.ResolveThumbColor();
    if (thumbColor.alpha > 0 && thumb.width > 0.0f && thumb.height > 0.0f) {
        FillRoundedRect(buffer, thumb.x, thumb.y, thumb.width, thumb.height, radius, thumbColor, opacity);
    }

    ScrollbarPaintResult result;
    result.bounds = bounds;
    result.advanceY = bounds.height + 16.0f * scale;
    return result;
}

}  // namespace Robo
