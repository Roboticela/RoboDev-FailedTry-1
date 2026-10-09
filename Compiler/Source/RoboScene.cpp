#include "Robo/RoboScene.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cstdint>

namespace Robo {

namespace {

void Downsample2x(const FrameBuffer& source, FrameBuffer& destination) {
    const int width = destination.GetWidth();
    const int height = destination.GetHeight();
    const std::uint8_t* src = source.GetPixels();
    std::uint8_t* dst = destination.GetPixels();
    const int srcWidth = source.GetWidth();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int sx = x * 2;
            const int sy = y * 2;
            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int a = 0;
            for (int oy = 0; oy < 2; ++oy) {
                for (int ox = 0; ox < 2; ++ox) {
                    const std::size_t index =
                        static_cast<std::size_t>(((sy + oy) * srcWidth + (sx + ox)) * 4);
                    r += src[index + 0];
                    g += src[index + 1];
                    b += src[index + 2];
                    a += src[index + 3];
                }
            }
            const std::size_t out = static_cast<std::size_t>((y * width + x) * 4);
            dst[out + 0] = static_cast<std::uint8_t>((r + 2) / 4);
            dst[out + 1] = static_cast<std::uint8_t>((g + 2) / 4);
            dst[out + 2] = static_cast<std::uint8_t>((b + 2) / 4);
            dst[out + 3] = static_cast<std::uint8_t>((a + 2) / 4);
        }
    }
}

void ScaleBounds(ButtonBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleBounds(InputBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

}  // namespace

void RoboScene::Clear() {
    items_.clear();
    hoveredIndex_ = -1;
    pressedIndex_ = -1;
    focusedInputIndex_ = -1;
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

void RoboScene::AddInput(Input input) {
    SceneItem item;
    item.kind = SceneItemKind::Input;
    item.input = std::move(input);
    items_.push_back(std::move(item));
}

bool RoboScene::RerenderIfPossible() {
    if (!hasRenderContext_ || !platform_ || !fonts_) {
        return false;
    }
    Render(*platform_, *fonts_, background_, padding_);
    return true;
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) {
    platform_ = &platform;
    fonts_ = &fonts;
    background_ = background;
    padding_ = padding;
    hasRenderContext_ = true;

    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    const float pixelScale = std::max(0.5f, platform.GetPixelScale());
    int superSample = 2;
    const long long hiResBytes =
        static_cast<long long>(width) * static_cast<long long>(height) * 4LL * 4LL;
    if (hiResBytes > 96LL * 1024LL * 1024LL) {
        superSample = 1;
    }
    const float paintScale = pixelScale * static_cast<float>(superSample);

    FrameBuffer hiRes(width * superSample, height * superSample);
    hiRes.Clear(background);

    const float pad = padding * paintScale;
    const float contentWidth = static_cast<float>(width * superSample) - pad * 2.0f;
    float cursorY = pad;
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Text) {
            cursorY += TextPainter::Paint(hiRes, fonts, item.text, pad, cursorY, contentWidth, paintScale);
        } else if (item.kind == SceneItemKind::Button) {
            const ButtonPaintResult painted =
                ButtonPainter::Paint(hiRes, fonts, item.button, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                ButtonBounds bounds = item.button.GetBounds();
                ScaleBounds(bounds, 1.0f / static_cast<float>(superSample));
                item.button.SetBounds(bounds);
            }
        } else if (item.kind == SceneItemKind::Input) {
            if (i == focusedInputIndex_) {
                item.input.SetInteraction(InputInteraction::Focused);
            } else if (i == hoveredIndex_ && item.input.GetInteraction() != InputInteraction::Focused) {
                item.input.SetInteraction(InputInteraction::Hovered);
            } else if (i != focusedInputIndex_) {
                item.input.SetInteraction(InputInteraction::Normal);
            }

            const InputPaintResult painted =
                InputPainter::Paint(hiRes, fonts, item.input, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                InputBounds bounds = item.input.GetBounds();
                ScaleBounds(bounds, 1.0f / static_cast<float>(superSample));
                item.input.SetBounds(bounds);
            }
        }
    }

    if (superSample > 1) {
        FrameBuffer output(width, height);
        Downsample2x(hiRes, output);
        platform.Present(output);
    } else {
        platform.Present(hiRes);
    }
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

int RoboScene::HitTestInput(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        if (items_[static_cast<std::size_t>(i)].kind != SceneItemKind::Input) {
            continue;
        }
        if (items_[static_cast<std::size_t>(i)].input.GetBounds().Contains(x, y)) {
            return i;
        }
    }
    return -1;
}

HitKind RoboScene::HitTestKind(float x, float y) const {
    if (HitTestButton(x, y) >= 0) {
        return HitKind::Button;
    }
    if (HitTestInput(x, y) >= 0) {
        return HitKind::Input;
    }
    return HitKind::None;
}

bool RoboScene::HitTest(float x, float y) const {
    return HitTestKind(x, y) != HitKind::None;
}

bool RoboScene::SetHoverIndex(int index) {
    if (hoveredIndex_ == index) {
        return false;
    }

    if (hoveredIndex_ >= 0 && static_cast<std::size_t>(hoveredIndex_) < items_.size()) {
        SceneItem& previous = items_[static_cast<std::size_t>(hoveredIndex_)];
        if (previous.kind == SceneItemKind::Button && pressedIndex_ != hoveredIndex_) {
            previous.button.SetInteraction(ButtonInteraction::Normal);
        } else if (previous.kind == SceneItemKind::Input && focusedInputIndex_ != hoveredIndex_) {
            previous.input.SetInteraction(InputInteraction::Normal);
        }
    }

    hoveredIndex_ = index;
    if (hoveredIndex_ >= 0 && pressedIndex_ != hoveredIndex_) {
        SceneItem& item = items_[static_cast<std::size_t>(hoveredIndex_)];
        if (item.kind == SceneItemKind::Button) {
            item.button.SetInteraction(ButtonInteraction::Hovered);
        } else if (item.kind == SceneItemKind::Input && focusedInputIndex_ != hoveredIndex_) {
            item.input.SetInteraction(InputInteraction::Hovered);
        }
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
    if (pressedIndex_ >= 0 && items_[static_cast<std::size_t>(pressedIndex_)].kind == SceneItemKind::Button) {
        items_[static_cast<std::size_t>(pressedIndex_)].button.SetInteraction(ButtonInteraction::Pressed);
        changed = true;
    }
    return changed;
}

bool RoboScene::SetFocusedInput(int index) {
    if (focusedInputIndex_ == index) {
        if (index >= 0) {
            items_[static_cast<std::size_t>(index)].input.SetInteraction(InputInteraction::Focused);
        }
        return false;
    }

    bool changed = false;
    if (focusedInputIndex_ >= 0 && static_cast<std::size_t>(focusedInputIndex_) < items_.size() &&
        items_[static_cast<std::size_t>(focusedInputIndex_)].kind == SceneItemKind::Input) {
        const InputInteraction next =
            (focusedInputIndex_ == hoveredIndex_) ? InputInteraction::Hovered : InputInteraction::Normal;
        items_[static_cast<std::size_t>(focusedInputIndex_)].input.SetInteraction(next);
        changed = true;
    }

    focusedInputIndex_ = index;
    if (focusedInputIndex_ >= 0) {
        Input& input = items_[static_cast<std::size_t>(focusedInputIndex_)].input;
        input.SetInteraction(InputInteraction::Focused);
        input.SetCaretIndex(input.GetValue().size());
        changed = true;
    }
    return changed;
}

bool RoboScene::PointerMove(float x, float y) {
    if (pressedIndex_ >= 0 && items_[static_cast<std::size_t>(pressedIndex_)].kind == SceneItemKind::Button) {
        const bool inside = HitTestButton(x, y) == pressedIndex_;
        const ButtonInteraction next = inside ? ButtonInteraction::Pressed : ButtonInteraction::Hovered;
        Button& button = items_[static_cast<std::size_t>(pressedIndex_)].button;
        if (button.GetInteraction() != next) {
            button.SetInteraction(next);
            RerenderIfPossible();
            return true;
        }
        return false;
    }

    int hover = HitTestButton(x, y);
    if (hover < 0) {
        hover = HitTestInput(x, y);
    }
    const bool changed = SetHoverIndex(hover);
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::PointerDown(float x, float y) {
    const int buttonIndex = HitTestButton(x, y);
    const int inputIndex = HitTestInput(x, y);

    bool changed = false;
    if (buttonIndex >= 0) {
        changed = SetFocusedInput(-1) || changed;
        changed = SetPressedIndex(buttonIndex) || changed;
        hoveredIndex_ = buttonIndex;
    } else if (inputIndex >= 0) {
        changed = SetPressedIndex(-1) || changed;
        changed = SetFocusedInput(inputIndex) || changed;
        hoveredIndex_ = inputIndex;
    } else {
        changed = SetPressedIndex(-1) || changed;
        changed = SetFocusedInput(-1) || changed;
    }

    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::PointerUp(float x, float y) {
    bool changed = SetPressedIndex(-1);
    int hover = HitTestButton(x, y);
    if (hover < 0) {
        hover = HitTestInput(x, y);
    }
    if (SetHoverIndex(hover)) {
        changed = true;
    }
    if (changed) {
        RerenderIfPossible();
    }
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
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::HasFocusedInput() const {
    return focusedInputIndex_ >= 0;
}

bool RoboScene::KeyInsert(const std::string& utf8) {
    if (focusedInputIndex_ < 0) {
        return false;
    }
    Input& input = items_[static_cast<std::size_t>(focusedInputIndex_)].input;
    if (!input.InsertText(utf8)) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyBackspace() {
    if (focusedInputIndex_ < 0) {
        return false;
    }
    Input& input = items_[static_cast<std::size_t>(focusedInputIndex_)].input;
    if (!input.DeleteBackward()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyDelete() {
    if (focusedInputIndex_ < 0) {
        return false;
    }
    Input& input = items_[static_cast<std::size_t>(focusedInputIndex_)].input;
    if (!input.DeleteForward()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyLeft() {
    if (focusedInputIndex_ < 0) {
        return false;
    }
    Input& input = items_[static_cast<std::size_t>(focusedInputIndex_)].input;
    if (!input.MoveCaretLeft()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyRight() {
    if (focusedInputIndex_ < 0) {
        return false;
    }
    Input& input = items_[static_cast<std::size_t>(focusedInputIndex_)].input;
    if (!input.MoveCaretRight()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyEscape() {
    if (focusedInputIndex_ < 0) {
        return false;
    }
    const bool changed = SetFocusedInput(-1);
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

}  // namespace Robo
