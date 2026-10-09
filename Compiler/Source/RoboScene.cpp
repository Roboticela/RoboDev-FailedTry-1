#include "Robo/RoboScene.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>

namespace Robo {

void RoboScene::Clear() {
    items_.clear();
    hoveredIndex_ = -1;
    pressedIndex_ = -1;
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

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) {
    platform_ = &platform;
    fonts_ = &fonts;
    background_ = background;
    padding_ = padding;
    hasRenderContext_ = true;

    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    FrameBuffer buffer(width, height);
    buffer.Clear(background);

    const float contentWidth = static_cast<float>(width) - padding * 2.0f;
    float cursorY = padding;
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Text) {
            cursorY += TextPainter::Paint(buffer, fonts, item.text, padding, cursorY, contentWidth);
        } else if (item.kind == SceneItemKind::Button) {
            const ButtonPaintResult painted =
                ButtonPainter::Paint(buffer, fonts, item.button, padding, cursorY, contentWidth);
            cursorY += painted.advanceY;
        }
    }

    platform.Present(buffer);
}

int RoboScene::HitTestButton(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        if (items_[static_cast<std::size_t>(i)].kind != SceneItemKind::Button) {
            continue;
        }
        if (items_[static_cast<std::size_t>(i)].button.GetBounds().Contains(x, y)) {
            return i;
        }
    }
    return -1;
}

bool RoboScene::SetHoverIndex(int index) {
    if (hoveredIndex_ == index) {
        return false;
    }

    if (hoveredIndex_ >= 0 && static_cast<std::size_t>(hoveredIndex_) < items_.size() &&
        items_[static_cast<std::size_t>(hoveredIndex_)].kind == SceneItemKind::Button &&
        pressedIndex_ != hoveredIndex_) {
        items_[static_cast<std::size_t>(hoveredIndex_)].button.SetInteraction(ButtonInteraction::Normal);
    }

    hoveredIndex_ = index;
    if (hoveredIndex_ >= 0 && pressedIndex_ != hoveredIndex_) {
        items_[static_cast<std::size_t>(hoveredIndex_)].button.SetInteraction(ButtonInteraction::Hovered);
    }
    return true;
}

bool RoboScene::SetPressedIndex(int index) {
    bool changed = false;
    if (pressedIndex_ >= 0 && static_cast<std::size_t>(pressedIndex_) < items_.size() &&
        items_[static_cast<std::size_t>(pressedIndex_)].kind == SceneItemKind::Button) {
        const ButtonInteraction next =
            (pressedIndex_ == hoveredIndex_) ? ButtonInteraction::Hovered : ButtonInteraction::Normal;
        items_[static_cast<std::size_t>(pressedIndex_)].button.SetInteraction(next);
        changed = true;
    }

    pressedIndex_ = index;
    if (pressedIndex_ >= 0) {
        items_[static_cast<std::size_t>(pressedIndex_)].button.SetInteraction(ButtonInteraction::Pressed);
        changed = true;
    }
    return changed;
}

bool RoboScene::PointerMove(float x, float y) {
    if (pressedIndex_ >= 0) {
        const bool inside = HitTestButton(x, y) == pressedIndex_;
        const ButtonInteraction next = inside ? ButtonInteraction::Pressed : ButtonInteraction::Hovered;
        Button& button = items_[static_cast<std::size_t>(pressedIndex_)].button;
        if (button.GetInteraction() != next) {
            button.SetInteraction(next);
            if (hasRenderContext_ && platform_ && fonts_) {
                Render(*platform_, *fonts_, background_, padding_);
            }
            return true;
        }
        return false;
    }

    const bool changed = SetHoverIndex(HitTestButton(x, y));
    if (changed && hasRenderContext_ && platform_ && fonts_) {
        Render(*platform_, *fonts_, background_, padding_);
    }
    return changed;
}

bool RoboScene::PointerDown(float x, float y) {
    const int index = HitTestButton(x, y);
    const bool changed = SetPressedIndex(index);
    if (index >= 0) {
        hoveredIndex_ = index;
    }
    if (changed && hasRenderContext_ && platform_ && fonts_) {
        Render(*platform_, *fonts_, background_, padding_);
    }
    return changed;
}

bool RoboScene::PointerUp(float x, float y) {
    const int released = pressedIndex_;
    bool changed = SetPressedIndex(-1);
    const int hover = HitTestButton(x, y);
    if (SetHoverIndex(hover)) {
        changed = true;
    }
    if (changed && hasRenderContext_ && platform_ && fonts_) {
        Render(*platform_, *fonts_, background_, padding_);
    }
    (void)released;
    return changed;
}

bool RoboScene::PointerLeave() {
    bool changed = false;
    if (pressedIndex_ >= 0) {
        changed = SetPressedIndex(-1) || changed;
    }
    if (hoveredIndex_ >= 0) {
        changed = SetHoverIndex(-1) || changed;
    }
    if (changed && hasRenderContext_ && platform_ && fonts_) {
        Render(*platform_, *fonts_, background_, padding_);
    }
    return changed;
}

bool RoboScene::HitTest(float x, float y) const {
    return HitTestButton(x, y) >= 0;
}

}  // namespace Robo
