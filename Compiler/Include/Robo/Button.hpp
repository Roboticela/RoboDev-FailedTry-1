#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "TextAlign.hpp"

#include <string>

namespace Robo {

enum class ButtonVariant {
    Filled,
    Tinted,
    Gray,
    Plain
};

struct ButtonStyle {
    ButtonVariant variant = ButtonVariant::Filled;
    double fontSize = 17.0;
    FontWeight fontWeight = FontWeight::SemiBold;
    FontStyleKind fontStyle = FontStyleKind::Normal;
    Color textColor = Color::FromHex("#FFFFFF");
    Color backgroundColor = Color::FromHex("#007AFF");
    double cornerRadius = 14.0;
    double paddingX = 20.0;
    double paddingY = 12.0;
    double letterSpacing = 0.0;
    double opacity = 1.0;
    TextAlign align = TextAlign::Left;
    bool hasCustomTextColor = false;
    bool hasCustomBackground = false;
};

class Button {
public:
    Button();

    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetStyle(const ButtonStyle& style);
    const ButtonStyle& GetStyle() const;

    void ApplyVariantDefaults();

private:
    std::string label_;
    ButtonStyle style_;
};

ButtonVariant ParseButtonVariant(const std::string& value);

}  // namespace Robo
