#pragma once

#include "Button.hpp"
#include "Flexbox.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Grid.hpp"
#include "IPlatform.hpp"
#include "Input.hpp"
#include "Position.hpp"
#include "Scrollbar.hpp"
#include "Text.hpp"

#include <string>
#include <vector>

namespace Robo {

enum class SceneItemKind {
    Text,
    Button,
    Input,
    Scrollbar,
    Flexbox,
    Grid,
    Position
};

enum class HitKind {
    None,
    Button,
    Input,
    Scrollbar
};

struct SceneItem {
    SceneItemKind kind = SceneItemKind::Text;
    Text text;
    Button button;
    Input input;
    Scrollbar scrollbar;
    Flexbox flexbox;
    Grid grid;
    Position position;
};

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void AddButton(Button button);
    void AddInput(Input input);
    void AddScrollbar(Scrollbar scrollbar);
    void AddFlexbox(Flexbox flexbox);
    void AddGrid(Grid grid);
    void AddPosition(Position position);
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
    Scrollbar* FindScrollbarAt(float x, float y);
    const Button* FindButtonAt(float x, float y) const;
    const Input* FindInputAt(float x, float y) const;
    const Scrollbar* FindScrollbarAt(float x, float y) const;

    bool SetHoveredButton(Button* button);
    bool SetHoveredInput(Input* input);
    bool SetHoveredScrollbar(Scrollbar* scrollbar);
    bool SetPressedButton(Button* button);
    bool SetFocusedInput(Input* input);
    bool RerenderIfPossible();

    std::vector<SceneItem> items_;
    Button* hoveredButton_ = nullptr;
    Input* hoveredInput_ = nullptr;
    Scrollbar* hoveredScrollbar_ = nullptr;
    Button* pressedButton_ = nullptr;
    Input* focusedInput_ = nullptr;
    Scrollbar* draggingScrollbar_ = nullptr;

    Color background_ = Color::FromHex("#F7F3EB");
    float padding_ = 28.0f;
    IPlatform* platform_ = nullptr;
    FontEngine* fonts_ = nullptr;
    bool hasRenderContext_ = false;
};

}  // namespace Robo
