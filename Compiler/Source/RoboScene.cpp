#include "Robo/RoboScene.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>

namespace Robo {

void RoboScene::Clear() {
    items_.clear();
}

void RoboScene::AddText(Text text) {
    SceneItem item;
    item.kind = SceneItemKind::Text;
    item.text = std::move(text);
    items_.push_back(std::move(item));
}

void RoboScene::AddButton(Button button) {
    SceneItem item;
    item.kind = SceneItemKind::Button;
    item.button = std::move(button);
    items_.push_back(std::move(item));
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) const {
    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    FrameBuffer buffer(width, height);
    buffer.Clear(background);

    const float contentWidth = static_cast<float>(width) - padding * 2.0f;
    float cursorY = padding;
    for (const SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Text) {
            cursorY += TextPainter::Paint(buffer, fonts, item.text, padding, cursorY, contentWidth);
        } else if (item.kind == SceneItemKind::Button) {
            cursorY += ButtonPainter::Paint(buffer, fonts, item.button, padding, cursorY, contentWidth);
        }
    }

    platform.Present(buffer);
}

}  // namespace Robo
