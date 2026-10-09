#include "Robo/FlexboxPainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace Robo {

namespace {

float ParseDimension(const std::string& value, float parentSize, float fallback) {
    if (value.empty() || value == "auto") {
        return fallback;
    }
    if (value.size() > 1 && value.back() == '%') {
        const float percent = static_cast<float>(std::atof(value.c_str()));
        return parentSize * (percent / 100.0f);
    }
    if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        return static_cast<float>(std::atof(value.c_str()) * 16.0);
    }
    if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        return static_cast<float>(std::atof(value.c_str()));
    }
    return static_cast<float>(std::atof(value.c_str()));
}

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

void StrokeRoundedRect(FrameBuffer& buffer,
                       float x,
                       float y,
                       float width,
                       float height,
                       float radius,
                       float strokeWidth,
                       const Color& color,
                       float opacity) {
    if (width <= 0.0f || height <= 0.0f || strokeWidth <= 0.0f || color.alpha == 0) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min({radius, width * 0.5f, height * 0.5f}));
    const float half = strokeWidth * 0.5f;
    const int x0 = static_cast<int>(std::floor(x - half));
    const int y0 = static_cast<int>(std::floor(y - half));
    const int x1 = static_cast<int>(std::ceil(x + width + half));
    const int y1 = static_cast<int>(std::ceil(y + height + half));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;

    auto DistanceToRoundedRect = [&](float fx, float fy) -> float {
        float cx = fx;
        float cy = fy;
        if (fx < left) {
            cx = left;
        } else if (fx > right) {
            cx = right;
        }
        if (fy < top) {
            cy = top;
        } else if (fy > bottom) {
            cy = bottom;
        }

        const float dx = fx - cx;
        const float dy = fy - cy;
        const float distToCenter = std::sqrt(dx * dx + dy * dy);
        return distToCenter - cappedRadius;
    };

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;

            float dist = 0.0f;
            if (fx >= left && fx <= right && fy >= y && fy <= y + height) {
                const float dTop = fy - y;
                const float dBottom = (y + height) - fy;
                dist = -std::min(dTop, dBottom);
            } else if (fy >= top && fy <= bottom && fx >= x && fx <= x + width) {
                const float dLeft = fx - x;
                const float dRight = (x + width) - fx;
                dist = -std::min(dLeft, dRight);
            } else {
                dist = DistanceToRoundedRect(fx, fy);
            }

            const float edgeDist = std::abs(dist + half) - half;
            if (edgeDist <= 0.0f) {
                buffer.BlendPixel(px, py, color, opacity);
            } else if (edgeDist < 1.0f) {
                buffer.BlendPixel(px, py, color, opacity * (1.0f - edgeDist));
            }
        }
    }
}

struct MeasuredFlexChild {
    std::size_t itemIndex = 0;
    float baseWidth = 0.0f;
    float baseHeight = 0.0f;
    float baseMain = 0.0f;
    float baseCross = 0.0f;
    float allocatedMain = 0.0f;
    float allocatedCross = 0.0f;
    float finalX = 0.0f;
    float finalY = 0.0f;
    float finalWidth = 0.0f;
    float finalHeight = 0.0f;
};

struct FlexLine {
    std::vector<MeasuredFlexChild> children;
    float lineCrossSize = 0.0f;
};

struct ComputedFlexLayout {
    float containerWidth = 0.0f;
    float containerHeight = 0.0f;
    std::vector<FlexLine> lines;
};

ComputedFlexLayout ComputeLayout(const FontEngine& fonts,
                                Flexbox& flexbox,
                                float containerWidth,
                                float pixelScale,
                                float forcedWidth,
                                float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    const FlexboxStyle& style = flexbox.GetStyle();

    float resolvedW = containerWidth;
    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (!style.width.empty()) {
        resolvedW = ParseDimension(style.width, containerWidth, containerWidth);
    }
    resolvedW = std::max(style.minWidth * scale, std::min(resolvedW, style.maxWidth * scale));

    const float padL = style.paddingLeft * scale;
    const float padR = style.paddingRight * scale;
    const float padT = style.paddingTop * scale;
    const float padB = style.paddingBottom * scale;

    const float innerW = std::max(0.0f, resolvedW - padL - padR);

    const bool isRow = (style.direction == FlexDirection::Row || style.direction == FlexDirection::RowReverse);
    const float mainGap = isRow ? flexbox.ResolveGapX(scale) : flexbox.ResolveGapY(scale);
    const float crossGap = isRow ? flexbox.ResolveGapY(scale) : flexbox.ResolveGapX(scale);

    std::vector<FlexboxItem>& items = flexbox.GetItems();
    std::vector<MeasuredFlexChild> measuredChildren;
    measuredChildren.reserve(items.size());

    for (std::size_t i = 0; i < items.size(); ++i) {
        FlexboxItem& item = items[i];
        MeasuredFlexChild child;
        child.itemIndex = i;

        float itemAvailableW = innerW;
        if (!item.props.width.empty()) {
            itemAvailableW = ParseDimension(item.props.width, innerW, innerW);
        }

        switch (item.kind) {
            case FlexItemKind::Text: {
                TextMeasureResult measured = TextPainter::Measure(fonts, item.text, itemAvailableW, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Button: {
                ButtonMeasureResult measured = ButtonPainter::Measure(fonts, item.button, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Input: {
                InputMeasureResult measured = InputPainter::Measure(fonts, item.input, itemAvailableW, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Flexbox: {
                if (item.flexbox) {
                    FlexboxMeasureResult measured =
                        FlexboxPainter::Measure(fonts, *item.flexbox, itemAvailableW, scale);
                    child.baseWidth = measured.width;
                    child.baseHeight = measured.height;
                }
                break;
            }
        }

        if (!item.props.width.empty()) {
            child.baseWidth = ParseDimension(item.props.width, innerW, child.baseWidth);
        }
        if (!item.props.height.empty()) {
            child.baseHeight = ParseDimension(item.props.height, 0.0f, child.baseHeight);
        }

        if (isRow) {
            child.baseMain = child.baseWidth;
            child.baseCross = child.baseHeight;
            if (item.props.flexBasis >= 0.0f) {
                child.baseMain = item.props.flexBasis * scale;
            } else if (!item.props.flexBasisString.empty()) {
                child.baseMain = ParseDimension(item.props.flexBasisString, innerW, child.baseMain);
            }
        } else {
            child.baseMain = child.baseHeight;
            child.baseCross = child.baseWidth;
            if (item.props.flexBasis >= 0.0f) {
                child.baseMain = item.props.flexBasis * scale;
            } else if (!item.props.flexBasisString.empty()) {
                child.baseMain = ParseDimension(item.props.flexBasisString, 0.0f, child.baseMain);
            }
        }

        child.allocatedMain = child.baseMain;
        child.allocatedCross = child.baseCross;
        measuredChildren.push_back(child);
    }

    std::vector<FlexLine> lines;
    if (measuredChildren.empty()) {
        ComputedFlexLayout emptyLayout;
        emptyLayout.containerWidth = resolvedW;
        emptyLayout.containerHeight = padT + padB;
        return emptyLayout;
    }

    const float mainCapacity = isRow ? innerW : 100000.0f;
    const bool allowWrap = (style.wrap == FlexWrap::Wrap || style.wrap == FlexWrap::WrapReverse);

    FlexLine currentLine;
    float currentLineMain = 0.0f;

    for (std::size_t i = 0; i < measuredChildren.size(); ++i) {
        const MeasuredFlexChild& child = measuredChildren[i];
        const float gapNeeded = currentLine.children.empty() ? 0.0f : mainGap;

        if (allowWrap && !currentLine.children.empty() &&
            (currentLineMain + gapNeeded + child.baseMain > mainCapacity)) {
            lines.push_back(currentLine);
            currentLine = FlexLine();
            currentLineMain = 0.0f;
        }

        currentLineMain += (currentLine.children.empty() ? 0.0f : mainGap) + child.baseMain;
        currentLine.children.push_back(child);
    }
    if (!currentLine.children.empty()) {
        lines.push_back(currentLine);
    }

    for (FlexLine& line : lines) {
        float sumBaseMain = 0.0f;
        float totalGrow = 0.0f;
        float totalShrink = 0.0f;
        for (const MeasuredFlexChild& child : line.children) {
            sumBaseMain += child.baseMain;
            const FlexItemProps& props = items[child.itemIndex].props;
            totalGrow += props.flexGrow;
            totalShrink += props.flexShrink * child.baseMain;
        }

        const std::size_t childCount = line.children.size();
        const float totalGaps = childCount > 1 ? static_cast<float>(childCount - 1) * mainGap : 0.0f;
        float freeMain = isRow ? (innerW - sumBaseMain - totalGaps) : 0.0f;

        if (freeMain > 0.0f && totalGrow > 0.0f) {
            for (MeasuredFlexChild& child : line.children) {
                const FlexItemProps& props = items[child.itemIndex].props;
                if (props.flexGrow > 0.0f) {
                    child.allocatedMain = child.baseMain + freeMain * (props.flexGrow / totalGrow);
                }
            }
            freeMain = 0.0f;
        } else if (freeMain < 0.0f && totalShrink > 0.0f) {
            for (MeasuredFlexChild& child : line.children) {
                const FlexItemProps& props = items[child.itemIndex].props;
                if (props.flexShrink > 0.0f) {
                    const float shrinkAmount =
                        (-freeMain) * (props.flexShrink * child.baseMain / totalShrink);
                    child.allocatedMain = std::max(10.0f * scale, child.baseMain - shrinkAmount);
                }
            }
            freeMain = 0.0f;
        }

        float maxCross = 0.0f;
        for (MeasuredFlexChild& child : line.children) {
            maxCross = std::max(maxCross, child.baseCross);
        }
        line.lineCrossSize = maxCross;

        for (MeasuredFlexChild& child : line.children) {
            const FlexItemProps& props = items[child.itemIndex].props;
            FlexAlign align = props.alignSelf != FlexAlign::Auto ? props.alignSelf : style.align;
            if (align == FlexAlign::Stretch) {
                child.allocatedCross = line.lineCrossSize;
            }
        }
    }

    if (style.wrap == FlexWrap::WrapReverse) {
        std::reverse(lines.begin(), lines.end());
    }

    float totalLinesCross = 0.0f;
    for (std::size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        if (lineIdx > 0) {
            totalLinesCross += crossGap;
        }
        totalLinesCross += lines[lineIdx].lineCrossSize;
    }

    float resolvedH = padT + padB + totalLinesCross;
    if (forcedHeight > 0.0f) {
        resolvedH = forcedHeight;
    } else if (!style.height.empty() && style.height != "auto") {
        resolvedH = ParseDimension(style.height, 0.0f, resolvedH);
    }
    resolvedH = std::max(style.minHeight * scale, std::min(resolvedH, style.maxHeight * scale));

    float currentCrossOffset = padT;

    for (FlexLine& line : lines) {
        float sumAllocatedMain = 0.0f;
        for (const MeasuredFlexChild& child : line.children) {
            sumAllocatedMain += child.allocatedMain;
        }

        const std::size_t childCount = line.children.size();
        const float totalGaps = childCount > 1 ? static_cast<float>(childCount - 1) * mainGap : 0.0f;
        const float remainingMain = std::max(0.0f, (isRow ? innerW : sumAllocatedMain) - sumAllocatedMain - totalGaps);

        float mainCursor = padL;
        float actualGap = mainGap;

        if (isRow && remainingMain > 0.0f) {
            switch (style.justify) {
                case FlexJustify::Center:
                    mainCursor += remainingMain * 0.5f;
                    break;
                case FlexJustify::FlexEnd:
                    mainCursor += remainingMain;
                    break;
                case FlexJustify::SpaceBetween:
                    if (childCount > 1) {
                        actualGap = mainGap + remainingMain / static_cast<float>(childCount - 1);
                    }
                    break;
                case FlexJustify::SpaceAround:
                    if (childCount > 0) {
                        const float margin = remainingMain / static_cast<float>(childCount * 2);
                        mainCursor += margin;
                        actualGap = mainGap + margin * 2.0f;
                    }
                    break;
                case FlexJustify::SpaceEvenly:
                    if (childCount > 0) {
                        const float space = remainingMain / static_cast<float>(childCount + 1);
                        mainCursor += space;
                        actualGap = mainGap + space;
                    }
                    break;
                case FlexJustify::FlexStart:
                default:
                    break;
            }
        }

        if (style.direction == FlexDirection::RowReverse ||
            style.direction == FlexDirection::ColumnReverse) {
            std::reverse(line.children.begin(), line.children.end());
        }

        for (MeasuredFlexChild& child : line.children) {
            const FlexItemProps& props = items[child.itemIndex].props;
            FlexAlign align = props.alignSelf != FlexAlign::Auto ? props.alignSelf : style.align;

            float crossOffset = currentCrossOffset;
            if (align == FlexAlign::Center) {
                crossOffset += (line.lineCrossSize - child.allocatedCross) * 0.5f;
            } else if (align == FlexAlign::FlexEnd) {
                crossOffset += (line.lineCrossSize - child.allocatedCross);
            }

            if (isRow) {
                child.finalX = mainCursor;
                child.finalY = crossOffset;
                child.finalWidth = child.allocatedMain;
                child.finalHeight = child.allocatedCross;
                mainCursor += child.allocatedMain + actualGap;
            } else {
                child.finalX = crossOffset;
                child.finalY = mainCursor;
                child.finalWidth = child.allocatedCross;
                child.finalHeight = child.allocatedMain;
                mainCursor += child.allocatedMain + actualGap;
            }
        }

        currentCrossOffset += line.lineCrossSize + crossGap;
    }

    ComputedFlexLayout layout;
    layout.containerWidth = resolvedW;
    layout.containerHeight = resolvedH;
    layout.lines = std::move(lines);
    return layout;
}

}  // namespace

FlexboxMeasureResult FlexboxPainter::Measure(const FontEngine& fonts,
                                            Flexbox& flexbox,
                                            float containerWidth,
                                            float pixelScale,
                                            float forcedWidth,
                                            float forcedHeight) {
    ComputedFlexLayout layout =
        ComputeLayout(fonts, flexbox, containerWidth, pixelScale, forcedWidth, forcedHeight);
    FlexboxMeasureResult result;
    result.width = layout.containerWidth;
    result.height = layout.containerHeight;
    return result;
}

FlexboxPaintResult FlexboxPainter::Paint(FrameBuffer& buffer,
                                        const FontEngine& fonts,
                                        Flexbox& flexbox,
                                        float originX,
                                        float originY,
                                        float containerWidth,
                                        float pixelScale,
                                        float forcedWidth,
                                        float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    const FlexboxStyle& style = flexbox.GetStyle();

    ComputedFlexLayout layout =
        ComputeLayout(fonts, flexbox, containerWidth, scale, forcedWidth, forcedHeight);

    const float containerX = originX + style.marginLeft * scale;
    const float containerY = originY + style.marginTop * scale;
    const float containerW = layout.containerWidth;
    const float containerH = layout.containerHeight;

    if (style.backgroundColor.alpha > 0) {
        FillRoundedRect(buffer,
                        containerX,
                        containerY,
                        containerW,
                        containerH,
                        style.cornerRadius * scale,
                        style.backgroundColor,
                        style.opacity);
    }

    if (style.borderWidth > 0.0f && style.borderColor.alpha > 0) {
        StrokeRoundedRect(buffer,
                          containerX,
                          containerY,
                          containerW,
                          containerH,
                          style.cornerRadius * scale,
                          style.borderWidth * scale,
                          style.borderColor,
                          style.opacity);
    }

    std::vector<FlexboxItem>& items = flexbox.GetItems();

    for (FlexLine& line : layout.lines) {
        for (MeasuredFlexChild& child : line.children) {
            FlexboxItem& item = items[child.itemIndex];
            const float childX = containerX + child.finalX;
            const float childY = containerY + child.finalY;
            const float childW = child.finalWidth;
            const float childH = child.finalHeight;

            switch (item.kind) {
                case FlexItemKind::Text: {
                    TextPainter::Paint(buffer, fonts, item.text, childX, childY, childW, scale);
                    break;
                }
                case FlexItemKind::Button: {
                    ButtonPainter::Paint(buffer,
                                         fonts,
                                         item.button,
                                         childX,
                                         childY,
                                         childW,
                                         scale,
                                         childW,
                                         childH);
                    break;
                }
                case FlexItemKind::Input: {
                    InputPainter::Paint(buffer,
                                        fonts,
                                        item.input,
                                        childX,
                                        childY,
                                        childW,
                                        scale,
                                        childW,
                                        childH);
                    break;
                }
                case FlexItemKind::Flexbox: {
                    if (item.flexbox) {
                        FlexboxPainter::Paint(buffer,
                                              fonts,
                                              *item.flexbox,
                                              childX,
                                              childY,
                                              childW,
                                              scale,
                                              childW,
                                              childH);
                    }
                    break;
                }
            }
        }
    }

    FlexboxBounds bounds;
    bounds.x = containerX;
    bounds.y = containerY;
    bounds.width = containerW;
    bounds.height = containerH;
    flexbox.SetBounds(bounds);

    FlexboxPaintResult result;
    result.bounds = bounds;
    result.advanceY = containerH + (style.marginBottom + 16.0f) * scale;
    return result;
}

}  // namespace Robo
