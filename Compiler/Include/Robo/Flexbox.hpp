#pragma once

#include "Button.hpp"
#include "Color.hpp"
#include "FontStyle.hpp"
#include "Input.hpp"
#include "Text.hpp"
#include "TextAlign.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Robo {

enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

enum class FlexJustify {
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class FlexAlign {
    Stretch,
    FlexStart,
    Center,
    FlexEnd,
    Auto
};

enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

enum class FlexItemKind {
    Text,
    Button,
    Input,
    Flexbox
};

struct FlexItemProps {
    float flexGrow = 0.0f;
    float flexShrink = 0.0f;
    float flexBasis = -1.0f;
    std::string flexBasisString;
    FlexAlign alignSelf = FlexAlign::Auto;
    std::string width;
    std::string height;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
};

class Flexbox;

struct FlexboxItem {
    FlexItemKind kind = FlexItemKind::Text;
    FlexItemProps props;
    Text text;
    Button button;
    Input input;
    std::shared_ptr<Flexbox> flexbox;
};

struct FlexboxBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

struct FlexboxStyle {
    FlexDirection direction = FlexDirection::Row;
    FlexJustify justify = FlexJustify::FlexStart;
    FlexAlign align = FlexAlign::Stretch;
    FlexWrap wrap = FlexWrap::NoWrap;
    float gap = 0.0f;
    float rowGap = -1.0f;
    float columnGap = -1.0f;

    std::string width = "100%";
    std::string height = "auto";
    float minWidth = 0.0f;
    float maxWidth = 100000.0f;
    float minHeight = 0.0f;
    float maxHeight = 100000.0f;

    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;

    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;

    Color backgroundColor = Color::FromRgba(0, 0, 0, 0);
    Color borderColor = Color::FromRgba(0, 0, 0, 0);
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    float opacity = 1.0f;
};

class Flexbox {
public:
    Flexbox();

    void SetStyle(const FlexboxStyle& style);
    const FlexboxStyle& GetStyle() const;
    FlexboxStyle& GetStyle();

    void AddText(Text text, const FlexItemProps& props = {});
    void AddButton(Button button, const FlexItemProps& props = {});
    void AddInput(Input input, const FlexItemProps& props = {});
    void AddFlexbox(Flexbox flexbox, const FlexItemProps& props = {});
    void AddItem(FlexboxItem item);

    const std::vector<FlexboxItem>& GetItems() const;
    std::vector<FlexboxItem>& GetItems();

    void SetBounds(const FlexboxBounds& bounds);
    const FlexboxBounds& GetBounds() const;

    float ResolveGapX(float pixelScale) const;
    float ResolveGapY(float pixelScale) const;

private:
    FlexboxStyle style_;
    std::vector<FlexboxItem> items_;
    FlexboxBounds bounds_;
};

FlexDirection ParseFlexDirection(const std::string& value);
FlexJustify ParseFlexJustify(const std::string& value);
FlexAlign ParseFlexAlign(const std::string& value);
FlexWrap ParseFlexWrap(const std::string& value);

std::string FlexDirectionToCss(FlexDirection direction);
std::string FlexJustifyToCss(FlexJustify justify);
std::string FlexAlignToCss(FlexAlign align);
std::string FlexWrapToCss(FlexWrap wrap);

}  // namespace Robo
