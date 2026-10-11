#include "Robo/ScrollbarPainter.hpp"
#include "Robo/Foundation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Robo {

namespace {

constexpr float MIN_VISIBLE_OPACITY = 0.004f;

void FillRoundedRect(FrameBuffer& buffer,
                     float x,
                     float y,
                     float width,
                     float height,
                     float radius,
                     const Color& color,
                     float opacity) {
    buffer.FillRoundedRect(x, y, width, height, radius, color, opacity);
    return;
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

float EdgeFunction(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

// Filled triangle with 3x3 sub-sampling so the tiny arrowheads stay smooth.
void FillTriangle(FrameBuffer& buffer,
                  float ax,
                  float ay,
                  float bx,
                  float by,
                  float cx,
                  float cy,
                  const Color& color,
                  float opacity) {
    if (color.alpha == 0 || opacity <= 0.0f) {
        return;
    }

    const float minX = std::min({ax, bx, cx});
    const float maxX = std::max({ax, bx, cx});
    const float minY = std::min({ay, by, cy});
    const float maxY = std::max({ay, by, cy});

    const int x0 = static_cast<int>(std::floor(minX));
    const int y0 = static_cast<int>(std::floor(minY));
    const int x1 = static_cast<int>(std::ceil(maxX));
    const int y1 = static_cast<int>(std::ceil(maxY));

    // Screen-space y grows downwards, so the winding is flipped for the
    // inside test; both orientations are accepted through the sign check.
    const float area = EdgeFunction(ax, ay, bx, by, cx, cy);
    if (std::fabs(area) < 0.0001f) {
        return;
    }
    const float orientation = area > 0.0f ? 1.0f : -1.0f;

    const float offsets[3] = {0.1667f, 0.5f, 0.8333f};
    constexpr float SAMPLE_COUNT = 9.0f;
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            int hits = 0;
            for (float oy : offsets) {
                for (float ox : offsets) {
                    const float sampleX = static_cast<float>(px) + ox;
                    const float sampleY = static_cast<float>(py) + oy;
                    const float e0 = EdgeFunction(ax, ay, bx, by, sampleX, sampleY) * orientation;
                    const float e1 = EdgeFunction(bx, by, cx, cy, sampleX, sampleY) * orientation;
                    const float e2 = EdgeFunction(cx, cy, ax, ay, sampleX, sampleY) * orientation;
                    if (e0 >= 0.0f && e1 >= 0.0f && e2 >= 0.0f) {
                        ++hits;
                    }
                }
            }
            if (hits == 0) {
                continue;
            }
            const float coverage = static_cast<float>(hits) / SAMPLE_COUNT;
            buffer.BlendPixel(px, py, color, opacity * coverage);
        }
    }
}

// Pixel-grid step the tiny arrowheads snap to. The framebuffer is usually drawn
// super-sampled, so an even integer scale gives crisp edges after downsampling.
float ArrowSnapStep(float scale) {
    const float rounded = std::round(scale);
    if (std::fabs(scale - rounded) > 0.01f) {
        return 1.0f;
    }
    const int whole = static_cast<int>(rounded);
    return (whole >= 2 && whole % 2 == 0) ? 2.0f : 1.0f;
}

float SnapCoordinate(float value, float step) {
    return std::round(value / step) * step;
}

float SignedDistanceToLine(float px, float py, float ax, float ay, float bx, float by) {
    const float vx = bx - ax;
    const float vy = by - ay;
    const float length = std::sqrt(vx * vx + vy * vy);
    if (length <= 0.001f) {
        return 0.0f;
    }
    return ((px - ax) * vy - (py - ay) * vx) / length;
}

// A solid triangle. Start points backward along the track; End points forward.
void PaintArrowHead(FrameBuffer& buffer,
                    const ScrollbarBounds& cap,
                    ScrollbarOrientation orientation,
                    ScrollbarArrow arrow,
                    const Color& color,
                    float opacity,
                    float scale) {
    (void)scale;
    if (cap.width <= 0.0f || cap.height <= 0.0f) {
        return;
    }

    const bool vertical = orientation == ScrollbarOrientation::Vertical;
    const float along = vertical ? cap.height : cap.width;
    const float cross = vertical ? cap.width : cap.height;
    if (along < 4.0f || cross < 4.0f) {
        return;
    }

    const float alongCenter = (vertical ? cap.y : cap.x) + along * 0.5f;
    const float crossCenter = (vertical ? cap.x : cap.y) + cross * 0.5f;
    const float mark = std::min(along, cross) * 0.42f;
    const float halfBase = mark * 0.72f;
    const float halfHeight = mark * 0.58f;
    const bool pointsForward = arrow == ScrollbarArrow::End;
    const float tipAlong = pointsForward ? alongCenter + halfHeight : alongCenter - halfHeight;
    const float baseAlong = pointsForward ? alongCenter - halfHeight : alongCenter + halfHeight;

    const auto toX = [vertical](float crossCoord, float alongCoord) {
        return vertical ? crossCoord : alongCoord;
    };
    const auto toY = [vertical](float crossCoord, float alongCoord) {
        return vertical ? alongCoord : crossCoord;
    };

    float ax = toX(crossCenter, tipAlong);
    float ay = toY(crossCenter, tipAlong);
    float bx = toX(crossCenter + halfBase, baseAlong);
    float by = toY(crossCenter + halfBase, baseAlong);
    float cx = toX(crossCenter - halfBase, baseAlong);
    float cy = toY(crossCenter - halfBase, baseAlong);
    const float winding = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    if (winding < 0.0f) {
        std::swap(bx, cx);
        std::swap(by, cy);
    }

    const int x0 = static_cast<int>(std::floor(std::min({ax, bx, cx}) - 1.0f));
    const int y0 = static_cast<int>(std::floor(std::min({ay, by, cy}) - 1.0f));
    const int x1 = static_cast<int>(std::ceil(std::max({ax, bx, cx}) + 1.0f));
    const int y1 = static_cast<int>(std::ceil(std::max({ay, by, cy}) + 1.0f));

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float sampleX = static_cast<float>(px) + 0.5f;
            const float sampleY = static_cast<float>(py) + 0.5f;
            const float inside = std::min({SignedDistanceToLine(sampleX, sampleY, ax, ay, bx, by),
                                           SignedDistanceToLine(sampleX, sampleY, bx, by, cx, cy),
                                           SignedDistanceToLine(sampleX, sampleY, cx, cy, ax, ay)});
            const float coverage = std::max(0.0f, std::min(1.0f, inside + 0.5f));
            if (coverage > 0.0f) {
                buffer.BlendPixel(px, py, color, opacity * coverage);
            }
        }
    }
}

}  // namespace

ScrollbarMeasureResult ScrollbarPainter::Measure(const Scrollbar& scrollbar,
                                                 float containerWidth,
                                                 float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    ScrollbarStyle style = scrollbar.GetStyle();
    Resolve(style);

    ScrollbarMeasureResult result;
    // A scrollbar that is invisible because nothing overflows must not reserve
    // any layout space at all.
    if (!scrollbar.IsVisible()) {
        return result;
    }

    const float thickness = static_cast<float>(style.thickness) * scale;
    float length = static_cast<float>(style.length) * scale;
    if (style.orientation == ScrollbarOrientation::Horizontal && containerWidth > 0.0f) {
        length = std::min(length, containerWidth);
    }
    length = std::max(0.0f, length);

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
    ScrollbarStyle style = scrollbar.GetStyle();
    Resolve(style);

    const ScrollbarMeasureResult natural = Measure(scrollbar, containerWidth, scale);

    const float trackWidth = forcedWidth > 0.0f ? forcedWidth : natural.width;
    const float trackHeight = forcedHeight > 0.0f ? forcedHeight : natural.height;

    const float trackX = originX;
    const float trackY = originY;

    ScrollbarBounds bounds;
    bounds.x = trackX;
    bounds.y = trackY;
    bounds.width = std::max(0.0f, trackWidth);
    bounds.height = std::max(0.0f, trackHeight);

    scrollbar.SetPixelScale(scale);
    scrollbar.SetBounds(bounds);

    ScrollbarPaintResult result;
    result.bounds = bounds;
    result.advanceY = bounds.height + 16.0f * scale;

    // Overflow-less bars collapse to zero bounds so hit testing ignores them.
    if (!scrollbar.IsVisible()) {
        ScrollbarBounds empty;
        empty.x = trackX;
        empty.y = trackY;
        scrollbar.SetBounds(empty);
        result.bounds = empty;
        result.advanceY = 0.0f;
        return result;
    }

    const float opacity = scrollbar.ResolveEffectiveOpacity();
    if (opacity <= MIN_VISIBLE_OPACITY) {
        // Auto-hidden: keep the geometry (so a hover can reveal it) but draw nothing.
        return result;
    }

    const bool vertical = style.orientation == ScrollbarOrientation::Vertical;
    const float laneCross = vertical ? bounds.width : bounds.height;
    const bool engaged = scrollbar.GetInteraction() != ScrollbarInteraction::Normal;
    const float visualCross = std::min(laneCross * (engaged ? 0.62f : 0.40f), (engaged ? 7.0f : 4.5f) * scale);
    // Pin the drawn bar to the outer edge of its lane so it sits against the container side.
    const float edgePad = 1.5f * scale;

    const Color trackColor = scrollbar.ResolveTrackColor();
    if (bounds.width > 0.0f && bounds.height > 0.0f) {
        const float travelStart = static_cast<float>(scrollbar.ResolveThumbTrackStart());
        const float travelLength = static_cast<float>(scrollbar.ResolveThumbTrackLength());
        const float trackCross = std::min(visualCross, std::max(2.0f * scale, laneCross * 0.28f));
        float trackX = bounds.x;
        float trackY = bounds.y;
        float trackW = bounds.width;
        float trackH = bounds.height;
        if (vertical) {
            trackX = bounds.x + bounds.width - trackCross - edgePad;
            trackW = trackCross;
            trackY += travelStart;
            trackH = travelLength;
        } else {
            trackY = bounds.y + bounds.height - trackCross - edgePad;
            trackH = trackCross;
            trackX += travelStart;
            trackW = travelLength;
        }
        const Color laneColor = style.hasCustomTrackColor ? trackColor : Color::FromRgba(60, 60, 67, 28);
        if (laneColor.alpha > 0 && trackW > 0.0f && trackH > 0.0f) {
            FillRoundedRect(buffer, trackX, trackY, trackW, trackH, trackCross * 0.5f, laneColor, opacity);
        }
    }

    const double arrowSize = scrollbar.ResolveArrowSize();
    if (arrowSize > 0.0) {
        const ScrollbarArrow arrowEnds[2] = {ScrollbarArrow::Start, ScrollbarArrow::End};
        for (ScrollbarArrow arrow : arrowEnds) {
            const ScrollbarBounds cap = scrollbar.ResolveArrowBounds(arrow);
            if (cap.width <= 0.0f || cap.height <= 0.0f) {
                continue;
            }

            const bool pressed = arrow == scrollbar.GetPressedArrow();
            const bool hot = arrow == scrollbar.GetHotArrow();
            const float markCross = std::min(laneCross, std::max(visualCross * 2.4f, 11.0f * scale));
            ScrollbarBounds mark = cap;
            if (vertical) {
                mark.width = markCross;
                mark.x = cap.x + cap.width - markCross - edgePad;
            } else {
                mark.height = markCross;
                mark.y = cap.y + cap.height - markCross - edgePad;
            }
            if (pressed || hot) {
                const float disk = std::min(mark.width, mark.height) * 0.92f;
                const float diskX = mark.x + (mark.width - disk) * 0.5f;
                const float diskY = mark.y + (mark.height - disk) * 0.5f;
                const Color wash = Color::FromRgba(0, 0, 0, pressed ? 40 : 22);
                FillRoundedRect(buffer, diskX, diskY, disk, disk, disk * 0.5f, wash, opacity);
            }

            PaintArrowHead(buffer,
                           mark,
                           style.orientation,
                           arrow,
                           scrollbar.ResolveArrowColor(arrow),
                           opacity,
                           scale);
        }
    }

    ScrollbarBounds thumb = scrollbar.ResolveThumbBounds();
    if (vertical) {
        thumb.x = bounds.x + bounds.width - visualCross - edgePad;
        thumb.width = visualCross;
    } else {
        thumb.y = bounds.y + bounds.height - visualCross - edgePad;
        thumb.height = visualCross;
    }
    const Color thumbColor = scrollbar.ResolveThumbColor();
    if (thumbColor.alpha > 0 && thumb.width > 0.0f && thumb.height > 0.0f) {
        const float thumbRadius = std::min(thumb.width, thumb.height) * 0.5f;
        FillRoundedRect(buffer, thumb.x, thumb.y, thumb.width, thumb.height, thumbRadius, thumbColor, opacity);
    }

    return result;
}

}  // namespace Robo
