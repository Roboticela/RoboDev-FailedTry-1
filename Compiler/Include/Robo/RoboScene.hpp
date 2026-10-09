#pragma once

#include "Button.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "IPlatform.hpp"
#include "Input.hpp"
#include "Text.hpp"

#include <string>
#include <vector>

namespace Robo {

enum class SceneItemKind {
    Text,
    Button,
    Input
};

enum class HitKind {
    None,
    Button,
    Input
};

struct SceneItem {
    SceneItemKind kind = SceneItemKind::Text;
    Text text;
    Button button;
    Input input;
};

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void AddButton(Button button);
    void AddInput(Input input);
    void Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding);

    bool PointerMove(float x, float y);
    bool PointerDown(float x, float y);
    bool PointerUp(float x, float y);
    bool PointerLeave();
    bool HitTest(float x, float y) const;
    HitKind HitTestKind(float x, float y) const;

    bool KeyInsert(const std::string& utf8);
    bool KeyBackspace();
    bool KeyDelete();
    bool KeyLeft();
    bool KeyRight();
    bool KeyEscape();
    bool HasFocusedInput() const;

private:
    int HitTestButton(float x, float y) const;
    int HitTestInput(float x, float y) const;
    bool SetHoverIndex(int index);
    bool SetPressedIndex(int index);
    bool SetFocusedInput(int index);
    bool RerenderIfPossible();

    std::vector<SceneItem> items_;
    int hoveredIndex_ = -1;
    int pressedIndex_ = -1;
    int focusedInputIndex_ = -1;
    Color background_ = Color::FromHex("#F7F3EB");
    float padding_ = 28.0f;
    IPlatform* platform_ = nullptr;
    FontEngine* fonts_ = nullptr;
    bool hasRenderContext_ = false;
};

}  // namespace Robo
