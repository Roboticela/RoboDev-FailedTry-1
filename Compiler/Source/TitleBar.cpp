#include "Robo/TitleBar.hpp"

namespace Robo {

void TitleBar::SetTitle(const std::string& title) {
    title_ = title;
}

const std::string& TitleBar::GetTitle() const {
    return title_;
}

}  // namespace Robo
