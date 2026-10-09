#include "Robo/Button.hpp"

namespace Robo {

Button::Button() = default;

void Button::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& Button::GetLabel() const {
    return label_;
}

void Button::SetStyle(const ButtonStyle& style) {
    style_ = style;
}

const ButtonStyle& Button::GetStyle() const {
    return style_;
}

void Button::ApplyVariantDefaults() {
    switch (style_.variant) {
        case ButtonVariant::Tinted:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromHex("#E5F1FF");
            }
            if (!style_.hasCustomTextColor) {
                style_.textColor = Color::FromHex("#007AFF");
            }
            break;
        case ButtonVariant::Gray:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromHex("#E5E5EA");
            }
            if (!style_.hasCustomTextColor) {
                style_.textColor = Color::FromHex("#1C1C1E");
            }
            break;
        case ButtonVariant::Plain:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromRgba(0, 0, 0, 0);
            }
            if (!style_.hasCustomTextColor) {
                style_.textColor = Color::FromHex("#007AFF");
            }
            style_.paddingX = style_.paddingX > 0.0 ? style_.paddingX : 8.0;
            style_.paddingY = style_.paddingY > 0.0 ? style_.paddingY : 8.0;
            break;
        case ButtonVariant::Filled:
        default:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromHex("#007AFF");
            }
            if (!style_.hasCustomTextColor) {
                style_.textColor = Color::FromHex("#FFFFFF");
            }
            break;
    }
}

ButtonVariant ParseButtonVariant(const std::string& value) {
    if (value == "tinted") {
        return ButtonVariant::Tinted;
    }
    if (value == "gray" || value == "grey") {
        return ButtonVariant::Gray;
    }
    if (value == "plain") {
        return ButtonVariant::Plain;
    }
    return ButtonVariant::Filled;
}

}  // namespace Robo
