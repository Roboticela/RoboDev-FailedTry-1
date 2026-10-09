#include "Robo/Flexbox.hpp"

#include <algorithm>

namespace Robo {

bool FlexboxBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointX <= (x + width) && pointY >= y && pointY <= (y + height);
}

Flexbox::Flexbox() = default;

void Flexbox::SetStyle(const FlexboxStyle& style) {
    style_ = style;
}

const FlexboxStyle& Flexbox::GetStyle() const {
    return style_;
}

FlexboxStyle& Flexbox::GetStyle() {
    return style_;
}

void Flexbox::AddText(Text text, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Text;
    item.text = std::move(text);
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddButton(Button button, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Button;
    item.button = std::move(button);
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddInput(Input input, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Input;
    item.input = std::move(input);
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddFlexbox(Flexbox flexbox, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Flexbox;
    item.flexbox = std::make_shared<Flexbox>(std::move(flexbox));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddItem(FlexboxItem item) {
    items_.push_back(std::move(item));
}

const std::vector<FlexboxItem>& Flexbox::GetItems() const {
    return items_;
}

std::vector<FlexboxItem>& Flexbox::GetItems() {
    return items_;
}

void Flexbox::SetBounds(const FlexboxBounds& bounds) {
    bounds_ = bounds;
}

const FlexboxBounds& Flexbox::GetBounds() const {
    return bounds_;
}

float Flexbox::ResolveGapX(float pixelScale) const {
    if (style_.columnGap >= 0.0f) {
        return style_.columnGap * pixelScale;
    }
    return style_.gap * pixelScale;
}

float Flexbox::ResolveGapY(float pixelScale) const {
    if (style_.rowGap >= 0.0f) {
        return style_.rowGap * pixelScale;
    }
    return style_.gap * pixelScale;
}

FlexDirection ParseFlexDirection(const std::string& value) {
    if (value == "column" || value == "col") {
        return FlexDirection::Column;
    }
    if (value == "column-reverse" || value == "col-reverse") {
        return FlexDirection::ColumnReverse;
    }
    if (value == "row-reverse") {
        return FlexDirection::RowReverse;
    }
    return FlexDirection::Row;
}

FlexJustify ParseFlexJustify(const std::string& value) {
    if (value == "center") {
        return FlexJustify::Center;
    }
    if (value == "flex-end" || value == "end" || value == "right") {
        return FlexJustify::FlexEnd;
    }
    if (value == "space-between" || value == "between") {
        return FlexJustify::SpaceBetween;
    }
    if (value == "space-around" || value == "around") {
        return FlexJustify::SpaceAround;
    }
    if (value == "space-evenly" || value == "evenly") {
        return FlexJustify::SpaceEvenly;
    }
    return FlexJustify::FlexStart;
}

FlexAlign ParseFlexAlign(const std::string& value) {
    if (value == "flex-start" || value == "start" || value == "top") {
        return FlexAlign::FlexStart;
    }
    if (value == "center" || value == "middle") {
        return FlexAlign::Center;
    }
    if (value == "flex-end" || value == "end" || value == "bottom") {
        return FlexAlign::FlexEnd;
    }
    if (value == "auto") {
        return FlexAlign::Auto;
    }
    return FlexAlign::Stretch;
}

FlexWrap ParseFlexWrap(const std::string& value) {
    if (value == "wrap") {
        return FlexWrap::Wrap;
    }
    if (value == "wrap-reverse") {
        return FlexWrap::WrapReverse;
    }
    return FlexWrap::NoWrap;
}

std::string FlexDirectionToCss(FlexDirection direction) {
    switch (direction) {
        case FlexDirection::Column:
            return "column";
        case FlexDirection::ColumnReverse:
            return "column-reverse";
        case FlexDirection::RowReverse:
            return "row-reverse";
        case FlexDirection::Row:
        default:
            return "row";
    }
}

std::string FlexJustifyToCss(FlexJustify justify) {
    switch (justify) {
        case FlexJustify::Center:
            return "center";
        case FlexJustify::FlexEnd:
            return "flex-end";
        case FlexJustify::SpaceBetween:
            return "space-between";
        case FlexJustify::SpaceAround:
            return "space-around";
        case FlexJustify::SpaceEvenly:
            return "space-evenly";
        case FlexJustify::FlexStart:
        default:
            return "flex-start";
    }
}

std::string FlexAlignToCss(FlexAlign align) {
    switch (align) {
        case FlexAlign::FlexStart:
            return "flex-start";
        case FlexAlign::Center:
            return "center";
        case FlexAlign::FlexEnd:
            return "flex-end";
        case FlexAlign::Auto:
            return "auto";
        case FlexAlign::Stretch:
        default:
            return "stretch";
    }
}

std::string FlexWrapToCss(FlexWrap wrap) {
    switch (wrap) {
        case FlexWrap::Wrap:
            return "wrap";
        case FlexWrap::WrapReverse:
            return "wrap-reverse";
        case FlexWrap::NoWrap:
        default:
            return "nowrap";
    }
}

}  // namespace Robo
