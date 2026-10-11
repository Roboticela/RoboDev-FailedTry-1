#include "Robo/Button.hpp"
#include "Robo/Foundation.hpp"

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
    if (style_.tokens.find("fontSize") == style_.tokens.end()) {
        style_.tokens["fontSize"] = "type.button.size";
    }
    if (style_.tokens.find("fontWeight") == style_.tokens.end()) {
        style_.tokens["fontWeight"] = "type.button.weight";
    }
    if (style_.tokens.find("letterSpacing") == style_.tokens.end()) {
        style_.tokens["letterSpacing"] = "type.button.tracking";
    }
    if (style_.tokens.find("cornerRadius") == style_.tokens.end()) {
        style_.tokens["cornerRadius"] = "radius.pill";
    }
    if (style_.tokens.find("paddingX") == style_.tokens.end()) {
        style_.tokens["paddingX"] = "space.4";
    }
    if (style_.tokens.find("paddingY") == style_.tokens.end()) {
        style_.tokens["paddingY"] = "space.3";
    }

    switch (style_.variant) {
        case ButtonVariant::Tinted:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.brand.primary.subtle";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.brand.primary";
            }
            break;
        case ButtonVariant::Gray:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.bg.tertiary";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.text.primary";
            }
            break;
        case ButtonVariant::Plain:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromRgba(0, 0, 0, 0);
                style_.tokens["backgroundColor"] = "";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.brand.primary";
            }
            break;
        case ButtonVariant::Filled:
        default:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.brand.primary";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.text.on-brand";
            }
            break;
    }
}

Color Button::ResolveBackgroundColor() const {
    ButtonStyle style = style_;
    Resolve(style);
    Color background = style.backgroundColor;
    const Color brand = ResolveColor(Color::FromHex("#0E7AFF"), "color.brand.primary");
    const Color onBrand = ResolveColor(Color::FromHex("#FFFFFF"), "color.text.on-brand");
    const Color subtle = ResolveColor(Color::FromHex("#0E7AFF1A"), "color.brand.primary.subtle");

    if (style.variant == ButtonVariant::Plain) {
        if (interaction_ == ButtonInteraction::Hovered || interaction_ == ButtonInteraction::Pressed) {
            return subtle;
        }
        return background;
    }

    if (interaction_ == ButtonInteraction::Hovered) {
        if (style.variant == ButtonVariant::Filled) {
            return MixToward(background, onBrand, 0.12f);
        }
        if (style.variant == ButtonVariant::Tinted) {
            return MixToward(background, brand, 0.12f);
        }
        return MixToward(background, Color::FromHex("#000000"), 0.08f);
    }

    if (interaction_ == ButtonInteraction::Pressed) {
        if (style.variant == ButtonVariant::Filled) {
            return MixToward(background, Color::FromHex("#000000"), 0.18f);
        }
        if (style.variant == ButtonVariant::Tinted) {
            return MixToward(background, brand, 0.22f);
        }
        return MixToward(background, Color::FromHex("#000000"), 0.16f);
    }

    return background;
}

Color Button::ResolveTextColor() const {
    ButtonStyle style = style_;
    Resolve(style);
    if (style.variant == ButtonVariant::Plain && interaction_ == ButtonInteraction::Pressed) {
        return MixToward(style.textColor, ResolveColor(style.textColor, "color.brand.primary.active"), 0.35f);
    }
    return style.textColor;
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
