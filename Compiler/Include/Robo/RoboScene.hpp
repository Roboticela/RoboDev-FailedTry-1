#pragma once

#include "Button.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "IPlatform.hpp"
#include "Text.hpp"

#include <vector>

namespace Robo {

enum class SceneItemKind {
    Text,
    Button
};

struct SceneItem {
    SceneItemKind kind = SceneItemKind::Text;
    Text text;
    Button button;
};

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void AddButton(Button button);
    void Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding);

    bool PointerMove(float x, float y);
    bool PointerDown(float x, float y);
    bool PointerUp(float x, float y);
    bool PointerLeave();
    bool HitTest(float x, float y) const;

private:
    int HitTestButton(float x, float y) const;
    bool SetHoverIndex(int index);
    bool SetPressedIndex(int index);

    std::vector<SceneItem> items_;
    int hoveredIndex_ = -1;
    int pressedIndex_ = -1;
    Color background_ = Color::FromHex("#F7F3EB");
    float padding_ = 28.0f;
    IPlatform* platform_ = nullptr;
    FontEngine* fonts_ = nullptr;
    bool hasRenderContext_ = false;
};

}  // namespace Robo
