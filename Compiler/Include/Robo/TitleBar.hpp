#pragma once

#include <string>

namespace Robo {

constexpr float TITLE_BAR_HEIGHT = 57.0f;

// Apple navigation bar. TitleBar and Navbar are the same control.
class TitleBar {
public:
    void SetTitle(const std::string& title);
    const std::string& GetTitle() const;

private:
    std::string title_;
};

}  // namespace Robo
