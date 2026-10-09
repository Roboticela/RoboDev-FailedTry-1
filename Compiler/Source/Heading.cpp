#include "Robo/Heading.hpp"

#include <algorithm>

namespace Robo {

int Heading::ClampLevel(int level) {
    return std::max(1, std::min(6, level));
}

TextStyle Heading::DefaultStyle(int level) {
    TextStyle style;
    style.wrap = true;
    style.width = "100%";
    style.align = TextAlign::Left;
    style.color = Color::FromHex("#102A43");
    style.fontWeight = FontWeight::SemiBold;
    style.fontStyle = FontStyleKind::Normal;
    style.letterSpacing = 0.0;

    switch (ClampLevel(level)) {
        case 1:
            style.fontSize = 40.0;
            style.lineHeight = 1.15;
            style.fontWeight = FontWeight::Bold;
            break;
        case 2:
            style.fontSize = 32.0;
            style.lineHeight = 1.2;
            style.fontWeight = FontWeight::SemiBold;
            break;
        case 3:
            style.fontSize = 28.0;
            style.lineHeight = 1.25;
            style.fontWeight = FontWeight::SemiBold;
            break;
        case 4:
            style.fontSize = 24.0;
            style.lineHeight = 1.3;
            style.fontWeight = FontWeight::SemiBold;
            break;
        case 5:
            style.fontSize = 20.0;
            style.lineHeight = 1.35;
            style.fontWeight = FontWeight::Medium;
            break;
        case 6:
        default:
            style.fontSize = 18.0;
            style.lineHeight = 1.4;
            style.fontWeight = FontWeight::Medium;
            style.color = Color::FromHex("#243B53");
            break;
    }
    return style;
}

Heading::Heading(int level) : level_(ClampLevel(level)) {
    SetStyle(DefaultStyle(level_));
}

int Heading::GetLevel() const {
    return level_;
}

}  // namespace Robo
