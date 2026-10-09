#include "Robo/RoboScene.hpp"
#include "Robo/ButtonPainter.hpp"
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

}  // namespace

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
    const float pixelScale = std::max(0.5f, platform.GetPixelScale());
    // 2x supersample for sharper glyphs; drop to 1x if the hi-res buffer would be huge.
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
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Text) {
            cursorY += TextPainter::Paint(hiRes, fonts, item.text, pad, cursorY, contentWidth, paintScale);
        } else if (item.kind == SceneItemKind::Button) {
            const ButtonPaintResult painted =
                ButtonPainter::Paint(hiRes, fonts, item.button, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;

            if (superSample > 1) {
                ButtonBounds bounds = item.button.GetBounds();
                const float inv = 1.0f / static_cast<float>(superSample);
                bounds.x *= inv;
                bounds.y *= inv;
                bounds.width *= inv;
                bounds.height *= inv;
                item.button.SetBounds(bounds);
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
    bool changed = SetPressedIndex(-1);
    if (SetHoverIndex(HitTestButton(x, y))) {
        changed = true;
    }
    if (changed && hasRenderContext_ && platform_ && fonts_) {
        Render(*platform_, *fonts_, background_, padding_);
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
    if (changed && hasRenderContext_ && platform_ && fonts_) {
        Render(*platform_, *fonts_, background_, padding_);
    }
    return changed;
}

bool RoboScene::HitTest(float x, float y) const {
    return HitTestButton(x, y) >= 0;
}

}  // namespace Robo
