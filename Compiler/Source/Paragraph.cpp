#include "Robo/Paragraph.hpp"

namespace Robo {

TextStyle Paragraph::DefaultStyle() {
    TextStyle style;
    style.fontSize = 17.0;
    style.color = Color::FromHex("#334E68");
    style.fontWeight = FontWeight::Regular;
    style.fontStyle = FontStyleKind::Normal;
    style.align = TextAlign::Left;
    style.wrap = true;
    style.lineHeight = 1.6;
    style.letterSpacing = 0.0;
    style.opacity = 1.0;
    style.width = "100%";
    style.maxWidth = "42rem";
    return style;
}

Paragraph::Paragraph() {
    SetStyle(DefaultStyle());
}

}  // namespace Robo
