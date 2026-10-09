#pragma once

#include "Button.hpp"
#include "Flexbox.hpp"
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
    Input,
    Flexbox
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
    Flexbox flexbox;
};

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void AddButton(Button button);
    void AddInput(Input input);
    void AddFlexbox(Flexbox flexbox);
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
    Button* FindButtonAt(float x, float y);
    Input* FindInputAt(float x, float y);
    const Button* FindButtonAt(float x, float y) const;
    const Input* FindInputAt(float x, float y) const;

    bool SetHoveredButton(Button* button);
    bool SetHoveredInput(Input* input);
    bool SetPressedButton(Button* button);
    bool SetFocusedInput(Input* input);
    bool RerenderIfPossible();

    std::vector<SceneItem> items_;
    Button* hoveredButton_ = nullptr;
    Input* hoveredInput_ = nullptr;
    Button* pressedButton_ = nullptr;
    Input* focusedInput_ = nullptr;

    Color background_ = Color::FromHex("#F7F3EB");
    float padding_ = 28.0f;
    IPlatform* platform_ = nullptr;
    FontEngine* fonts_ = nullptr;
    bool hasRenderContext_ = false;
};

}  // namespace Robo
