#include "Robo/ScrollbarPainter.hpp"

#include <algorithm>
#include <cmath>

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

float DistanceToSegment(float px, float py, float ax, float ay, float bx, float by) {
    const float vx = bx - ax;
    const float vy = by - ay;
    const float lengthSq = vx * vx + vy * vy;
    const float t = lengthSq <= 0.0001f
                        ? 0.0f
                        : std::max(0.0f, std::min(1.0f, ((px - ax) * vx + (py - ay) * vy) / lengthSq));
    const float dx = px - (ax + t * vx);
    const float dy = py - (ay + t * vy);
    return std::sqrt(dx * dx + dy * dy);
}

// A hairline chevron, the same mark macOS uses on a thin scroller.
void PaintArrowHead(FrameBuffer& buffer,
                    const ScrollbarBounds& cap,
                    ScrollbarOrientation orientation,
                    ScrollbarArrow arrow,
                    const Color& color,
                    float opacity,
                    float scale) {
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
    const float reach = std::min(along, cross) * 0.22f;
    const float wing = std::min(along, cross) * 0.24f;
    const bool pointsForward = arrow == ScrollbarArrow::End;
    const float tipAlong = pointsForward ? alongCenter + reach : alongCenter - reach;
    const float baseAlong = pointsForward ? alongCenter - reach * 0.15f : alongCenter + reach * 0.15f;
    const float stroke = std::max(1.35f * scale, std::min(cross, along) * 0.11f);

    const auto toX = [vertical](float crossCoord, float alongCoord) {
        return vertical ? crossCoord : alongCoord;
    };
    const auto toY = [vertical](float crossCoord, float alongCoord) {
        return vertical ? alongCoord : crossCoord;
    };

    const float tipX = toX(crossCenter, tipAlong);
    const float tipY = toY(crossCenter, tipAlong);
    const float leftX = toX(crossCenter - wing, baseAlong);
    const float leftY = toY(crossCenter - wing, baseAlong);
    const float rightX = toX(crossCenter + wing, baseAlong);
    const float rightY = toY(crossCenter + wing, baseAlong);

    const int x0 = static_cast<int>(std::floor(std::min({tipX, leftX, rightX}) - stroke));
    const int y0 = static_cast<int>(std::floor(std::min({tipY, leftY, rightY}) - stroke));
    const int x1 = static_cast<int>(std::ceil(std::max({tipX, leftX, rightX}) + stroke));
    const int y1 = static_cast<int>(std::ceil(std::max({tipY, leftY, rightY}) + stroke));

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float sampleX = static_cast<float>(px) + 0.5f;
            const float sampleY = static_cast<float>(py) + 0.5f;
            const float distance = std::min(DistanceToSegment(sampleX, sampleY, leftX, leftY, tipX, tipY),
                                            DistanceToSegment(sampleX, sampleY, rightX, rightY, tipX, tipY));
            const float coverage = std::max(0.0f, std::min(1.0f, stroke * 0.5f + 0.65f - distance));
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
    const ScrollbarStyle& style = scrollbar.GetStyle();

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
    const ScrollbarStyle& style = scrollbar.GetStyle();

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
    const float crossInset = std::max(0.0f, (laneCross - visualCross) * 0.5f);

    const Color trackColor = scrollbar.ResolveTrackColor();
    if (bounds.width > 0.0f && bounds.height > 0.0f) {
        const float travelStart = static_cast<float>(scrollbar.ResolveThumbTrackStart());
        const float travelLength = static_cast<float>(scrollbar.ResolveThumbTrackLength());
        const float trackCross = std::min(visualCross, std::max(2.0f * scale, laneCross * 0.28f));
        const float trackInset = std::max(0.0f, (laneCross - trackCross) * 0.5f);
        float trackX = bounds.x;
        float trackY = bounds.y;
        float trackW = bounds.width;
        float trackH = bounds.height;
        if (vertical) {
            trackX += trackInset;
            trackW = trackCross;
            trackY += travelStart;
            trackH = travelLength;
        } else {
            trackY += trackInset;
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
            if (pressed || hot) {
                const float disk = std::min(cap.width, cap.height) * 0.92f;
                const float diskX = cap.x + (cap.width - disk) * 0.5f;
                const float diskY = cap.y + (cap.height - disk) * 0.5f;
                const Color wash = Color::FromRgba(0, 0, 0, pressed ? 40 : 22);
                FillRoundedRect(buffer, diskX, diskY, disk, disk, disk * 0.5f, wash, opacity);
            }

            PaintArrowHead(buffer,
                           cap,
                           style.orientation,
                           arrow,
                           scrollbar.ResolveArrowColor(arrow),
                           opacity,
                           scale);
        }
    }

    ScrollbarBounds thumb = scrollbar.ResolveThumbBounds();
    if (vertical) {
        thumb.x += crossInset;
        thumb.width = visualCross;
    } else {
        thumb.y += crossInset;
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
