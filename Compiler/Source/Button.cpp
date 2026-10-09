#include "Robo/Button.hpp"

#include <algorithm>

namespace Robo {

namespace {

Color MixToward(const Color& from, const Color& toward, float amount) {
    const float t = std::max(0.0f, std::min(1.0f, amount));
    Color result;
    result.red = static_cast<std::uint8_t>(from.red + (toward.red - from.red) * t + 0.5f);
    result.green = static_cast<std::uint8_t>(from.green + (toward.green - from.green) * t + 0.5f);
    result.blue = static_cast<std::uint8_t>(from.blue + (toward.blue - from.blue) * t + 0.5f);
    result.alpha = static_cast<std::uint8_t>(from.alpha + (toward.alpha - from.alpha) * t + 0.5f);
    return result;
}

}  // namespace

bool ButtonBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

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

void Button::SetInteraction(ButtonInteraction interaction) {
    interaction_ = interaction;
}

ButtonInteraction Button::GetInteraction() const {
    return interaction_;
}

void Button::SetBounds(const ButtonBounds& bounds) {
    bounds_ = bounds;
}

const ButtonBounds& Button::GetBounds() const {
    return bounds_;
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

Color Button::ResolveBackgroundColor() const {
    Color background = style_.backgroundColor;

    if (style_.variant == ButtonVariant::Plain) {
        if (interaction_ == ButtonInteraction::Hovered) {
            return Color::FromHex("#E5F1FF");
        }
        if (interaction_ == ButtonInteraction::Pressed) {
            return Color::FromHex("#D6E9FF");
        }
        return background;
    }

    if (interaction_ == ButtonInteraction::Hovered) {
        if (style_.variant == ButtonVariant::Filled) {
            return MixToward(background, Color::FromHex("#FFFFFF"), 0.12f);
        }
        if (style_.variant == ButtonVariant::Tinted) {
            return MixToward(background, Color::FromHex("#007AFF"), 0.12f);
        }
        return MixToward(background, Color::FromHex("#000000"), 0.08f);
    }

    if (interaction_ == ButtonInteraction::Pressed) {
        if (style_.variant == ButtonVariant::Filled) {
            return MixToward(background, Color::FromHex("#000000"), 0.18f);
        }
        if (style_.variant == ButtonVariant::Tinted) {
            return MixToward(background, Color::FromHex("#007AFF"), 0.22f);
        }
        return MixToward(background, Color::FromHex("#000000"), 0.16f);
    }

    return background;
}

Color Button::ResolveTextColor() const {
    if (style_.variant == ButtonVariant::Plain && interaction_ == ButtonInteraction::Pressed) {
        return MixToward(style_.textColor, Color::FromHex("#003D80"), 0.2f);
    }
    if (interaction_ == ButtonInteraction::Pressed && style_.variant == ButtonVariant::Filled) {
        return MixToward(style_.textColor, Color::FromHex("#FFFFFF"), 0.0f);
    }
    return style_.textColor;
}

float Button::ResolveOpacity() const {
    if (interaction_ == ButtonInteraction::Pressed) {
        return static_cast<float>(style_.opacity) * 0.92f;
    }
    return static_cast<float>(style_.opacity);
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
