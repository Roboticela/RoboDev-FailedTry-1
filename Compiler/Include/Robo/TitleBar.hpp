#pragma once

#include "BrandLogo.hpp"
#include "ThemeIconToggle.hpp"

#include <string>

namespace Robo {

constexpr float TITLE_BAR_HEIGHT = 57.0f;

// Apple navigation bar. TitleBar and Navbar are the same control.
// The mark sits on the left. The split theme control sits on the right.
class TitleBar {
public:
    TitleBar();

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const;

    BrandLogo& Logo();
    const BrandLogo& Logo() const;
    ThemeIconToggle& ThemeIcon();
    const ThemeIconToggle& ThemeIcon() const;

private:
    std::string title_;
    BrandLogo logo_;
    ThemeIconToggle themeIcon_;
};

}  // namespace Robo
