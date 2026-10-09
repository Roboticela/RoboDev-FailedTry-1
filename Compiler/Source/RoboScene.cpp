#include "Robo/RoboScene.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>

namespace Robo {

void RoboScene::Clear() {
    texts_.clear();
}

void RoboScene::AddText(Text text) {
    texts_.push_back(std::move(text));
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) const {
    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    FrameBuffer buffer(width, height);
    buffer.Clear(background);

    const float contentWidth = static_cast<float>(width) - padding * 2.0f;
    float cursorY = padding;
    for (const Text& text : texts_) {
        cursorY += TextPainter::Paint(buffer, fonts, text, padding, cursorY, contentWidth);
    }

    platform.Present(buffer);
}

}  // namespace Robo
