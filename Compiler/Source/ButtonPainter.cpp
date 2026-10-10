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
    buffer.FillRoundedRect(x, y, width, height, radius, color, opacity);
    return;
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

ButtonMeasureResult ButtonPainter::Measure(const FontEngine& fonts,
                                           const Button& button,
                                           float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const ButtonStyle& style = button.GetStyle();
    const std::string label = CollapseToSingleLine(button.GetLabel());
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize) * scale;
    const float letterSpacing = static_cast<float>(style.letterSpacing) * scale;

    const float textWidth = fonts.MeasureWidth(label, fontSize, faceId, letterSpacing);
    const float ascent = fonts.GetAscent(fontSize, faceId);
    const float descent = fonts.GetDescent(fontSize, faceId);
    const float textHeight = std::max(fontSize, ascent + descent);

    const float paddingX = static_cast<float>(style.paddingX) * scale;
    const float paddingY = static_cast<float>(style.paddingY) * scale;

    ButtonMeasureResult result;
    result.width = textWidth + paddingX * 2.0f;
    result.height = textHeight + paddingY * 2.0f;
    result.advanceY = result.height + 16.0f * scale;
    return result;
}

ButtonPaintResult ButtonPainter::Paint(FrameBuffer& buffer,
                                       const FontEngine& fonts,
                                       Button& button,
                                       float originX,
                                       float originY,
                                       float containerWidth,
                                       float pixelScale,
                                       float forcedWidth,
                                       float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    const ButtonStyle& style = button.GetStyle();
    const std::string label = CollapseToSingleLine(button.GetLabel());
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize) * scale;
    const float letterSpacing = static_cast<float>(style.letterSpacing) * scale;
    const float opacity = button.ResolveOpacity();
    const Color backgroundColor = button.ResolveBackgroundColor();
    const Color textColor = button.ResolveTextColor();

    const float textWidth = fonts.MeasureWidth(label, fontSize, faceId, letterSpacing);
    const float ascent = fonts.GetAscent(fontSize, faceId);
    const float descent = fonts.GetDescent(fontSize, faceId);
    const float textHeight = std::max(fontSize, ascent + descent);

    const float paddingX = static_cast<float>(style.paddingX) * scale;
    const float paddingY = static_cast<float>(style.paddingY) * scale;
    const float naturalWidth = textWidth + paddingX * 2.0f;
    const float naturalHeight = textHeight + paddingY * 2.0f;

    const float buttonWidth = forcedWidth > 0.0f ? forcedWidth : naturalWidth;
    const float buttonHeight = forcedHeight > 0.0f ? forcedHeight : naturalHeight;

    float buttonX = originX;
    if (forcedWidth <= 0.0f) {
        if (style.align == TextAlign::Center) {
            buttonX = originX + std::max(0.0f, (containerWidth - buttonWidth) * 0.5f);
        } else if (style.align == TextAlign::Right) {
            buttonX = originX + std::max(0.0f, containerWidth - buttonWidth);
        }
    }

    const float buttonY = originY;
    const float pressOffset = button.GetInteraction() == ButtonInteraction::Pressed ? 1.0f * scale : 0.0f;

    if (backgroundColor.alpha > 0) {
        FillRoundedRect(buffer,
                        buttonX,
                        buttonY + pressOffset,
                        buttonWidth,
                        buttonHeight,
                        static_cast<float>(style.cornerRadius) * scale,
                        backgroundColor,
                        opacity);
    }

    float textX = buttonX + paddingX;
    if (buttonWidth > naturalWidth) {
        if (style.align == TextAlign::Center || style.variant != ButtonVariant::Plain) {
            textX = buttonX + (buttonWidth - textWidth) * 0.5f;
        } else if (style.align == TextAlign::Right) {
            textX = buttonX + buttonWidth - paddingX - textWidth;
        }
    }
    float baseline = buttonY + paddingY + ascent + pressOffset;
    if (buttonHeight > naturalHeight) {
        baseline = buttonY + (buttonHeight - textHeight) * 0.5f + ascent + pressOffset;
    }

    fonts.DrawString(buffer,
                     textX,
                     baseline,
                     label,
                     fontSize,
                     faceId,
                     textColor,
                     opacity,
                     letterSpacing);

    ButtonPaintResult result;
    result.bounds.x = buttonX;
    result.bounds.y = buttonY;
    result.bounds.width = buttonWidth;
    result.bounds.height = buttonHeight + pressOffset;
    result.advanceY = buttonHeight + 16.0f * scale;
    button.SetBounds(result.bounds);
    return result;
}

}  // namespace Robo
