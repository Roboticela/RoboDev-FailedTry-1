#include "Robo/TitleBar.hpp"

namespace Robo {

TitleBar::TitleBar() {
    logo_.SetSize(28.0f);
    themeIcon_.SetVariant(ThemeIconToggleVariant::Split);
}

void TitleBar::SetTitle(const std::string& title) {
    title_ = title;
}

const std::string& TitleBar::GetTitle() const {
    return title_;
}

BrandLogo& TitleBar::Logo() {
    return logo_;
}

const BrandLogo& TitleBar::Logo() const {
    return logo_;
}

ThemeIconToggle& TitleBar::ThemeIcon() {
    return themeIcon_;
}

const ThemeIconToggle& TitleBar::ThemeIcon() const {
    return themeIcon_;
}

}  // namespace Robo
