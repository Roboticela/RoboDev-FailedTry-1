#include "Robo/ButtonPainter.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace Robo {

namespace {

std::string CollapseToSingleLine(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    bool pendingSpace = false;
    for (unsigned char ch : input) {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v') {
            pendingSpace = true;
            continue;
        }
        if (pendingSpace && !output.empty()) {
            output.push_back(' ');
        }
        pendingSpace = false;
        output.push_back(static_cast<char>(ch));
    }
    return output;
}

void FillRoundedRect(FrameBuffer& buffer,
                     float x,
                     float y,
                     float width,
                     float height,
                     float radius,
                     const Color& color,
                     float opacity) {
    if (width <= 0.0f || height <= 0.0f) {
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
                } else {
                    cx = fx;
                    cy = fy;
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

float ButtonPainter::Paint(FrameBuffer& buffer,
                           const FontEngine& fonts,
                           const Button& button,
                           float originX,
                           float originY,
                           float containerWidth) {
    const ButtonStyle& style = button.GetStyle();
    const std::string label = CollapseToSingleLine(button.GetLabel());
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize);
    const float letterSpacing = static_cast<float>(style.letterSpacing);
    const float opacity = static_cast<float>(style.opacity);

    const float textWidth = fonts.MeasureWidth(label, fontSize, faceId, letterSpacing);
    const float ascent = fonts.GetAscent(fontSize, faceId);
    const float descent = fonts.GetDescent(fontSize, faceId);
    const float textHeight = std::max(fontSize, ascent + descent);

    const float paddingX = static_cast<float>(style.paddingX);
    const float paddingY = static_cast<float>(style.paddingY);
    const float buttonWidth = textWidth + paddingX * 2.0f;
    const float buttonHeight = textHeight + paddingY * 2.0f;

    float buttonX = originX;
    if (style.align == TextAlign::Center) {
        buttonX = originX + std::max(0.0f, (containerWidth - buttonWidth) * 0.5f);
    } else if (style.align == TextAlign::Right) {
        buttonX = originX + std::max(0.0f, containerWidth - buttonWidth);
    }

    const float buttonY = originY;

    if (style.backgroundColor.alpha > 0) {
        FillRoundedRect(buffer,
                        buttonX,
                        buttonY,
                        buttonWidth,
                        buttonHeight,
                        static_cast<float>(style.cornerRadius),
                        style.backgroundColor,
                        opacity);
    }

    const float textX = buttonX + paddingX;
    const float baseline = buttonY + paddingY + ascent;
    fonts.DrawString(buffer,
                     textX,
                     baseline,
                     label,
                     fontSize,
                     faceId,
                     style.textColor,
                     opacity,
                     letterSpacing);

    return buttonHeight + 16.0f;
}

}  // namespace Robo
