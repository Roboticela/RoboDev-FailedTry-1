#pragma once

#include "BrandLogo.hpp"
#include "Button.hpp"
#include "Container.hpp"
#include "Flexbox.hpp"
#include "FontEngine.hpp"
#include "Foundation.hpp"
#include "FrameBuffer.hpp"
#include "Grid.hpp"
#include "IPlatform.hpp"
#include "Input.hpp"
#include "Parse.hpp"
#include "Position.hpp"
#include "Scrollbar.hpp"
#include "Svg.hpp"
#include "Text.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Robo {

enum class SceneItemKind {
    Text,
    Button,
    Input,
    Scrollbar,
    Container,
    Flexbox,
    Grid,
    Position,
    Svg,
    BrandLogo
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
    std::shared_ptr<Container> container;
    Flexbox flexbox;
    Grid grid;
    Position position;
    Svg svg;
    BrandLogo brandLogo;
};

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void AddButton(Button button);
    void AddInput(Input input);
    void AddScrollbar(Scrollbar scrollbar);
    void AddContainer(Container container);
    void AddFlexbox(Flexbox flexbox);
    void AddGrid(Grid grid);
    void AddPosition(Position position);
    void AddSvg(Svg svg);
    void AddBrandLogo(BrandLogo logo);
    void Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding);

    void SetFoundation(const std::string& name);
    void SetFoundationForTheme(Theme theme, const std::string& name);
    void SetTheme(Theme theme);
    const Foundation& ActiveFoundation() const;

    bool PointerMove(float x, float y);
    bool PointerDown(float x, float y, bool fromTouch = false, int clicks = 1);
    bool LongPressSelect(float x, float y);
    bool PointerUp(float x, float y);
    bool PointerLeave();

    // Wheel / touchpad scrolling at a position, in device pixels.
    bool Wheel(float x, float y, float deltaX, float deltaY);
    // Keyboard scrolling applied to the focused or hovered scroll target.
    bool KeyScrollLine(int direction);
    bool KeyScrollPage(int direction);
    bool KeyScrollToStart();
    bool KeyScrollToEnd();
    // 0 none, 1 vertical, 2 horizontal. The focused bar wins over the hovered one.
    int ActiveScrollbarAxis() const;
    // True when a finger at this point should move a scrollbar instead of the page.
    bool OwnsScrollGesture(float x, float y);
    // Moves the page when the pointer is not over a scrolling region.
    bool ScrollBy(float deltaY);
    bool ScrollByX(float deltaX);
    // App zoom. The browser's own zoom stays disabled. Anchor is a point in the
    // current surface, in device pixels, which stays under the pointer.
    bool ZoomBy(float factor, float anchorX, float anchorY);
    bool ZoomTo(float zoom, float anchorX, float anchorY);
    float GetZoom() const;
    bool HitTest(float x, float y) const;
    HitKind HitTestKind(float x, float y) const;

    // Advances auto-hide / reveal animations; returns true when a repaint happened.
    bool Tick(double deltaMs);
    // True while any control still needs animation frames.
    bool NeedsTick();
    bool NeedsRender() const;

    bool KeyInsert(const std::string& utf8);
    bool KeyBackspace();
    bool KeyDelete();
    bool KeyLeft();
    bool KeyRight();
    bool KeyEscape();
    bool HasFocusedInput() const;
    bool SelectableAt(float x, float y) const;
    bool HasCaret() const;
    bool MoveSelection(int command, bool extend);
    bool CopySelection();
    bool SelectAllText();

private:
    Button* FindButtonAt(float x, float y);
    Input* FindInputAt(float x, float y);
    Scrollbar* FindScrollbarAt(float x, float y);
    Scrollbar* FindScrollbarNear(float x, float y, float tolerance);
    Container* FindContainerAt(float x, float y);
    Scrollbar* ScrollTargetAt(float x, float y);
    Scrollbar* ActiveScrollbar();
    const Button* FindButtonAt(float x, float y) const;
    const Input* FindInputAt(float x, float y) const;
    const Scrollbar* FindScrollbarAt(float x, float y) const;

    bool SetHoveredButton(Button* button);
    bool SetHoveredInput(Input* input);
    bool SetHoveredScrollbar(Scrollbar* scrollbar);
    bool SetFocusedScrollbar(Scrollbar* scrollbar);
    bool SetPressedButton(Button* button);
    bool SetFocusedInput(Input* input);
    bool RerenderIfPossible();
    void MarkFullDirty();
    void MarkDirty(float x, float y, float width, float height);
    void DirtyButton(const Button* button);
    void DirtyInput(const Input* input);
    void DirtyScrollbar(const Scrollbar* scrollbar);
    void DirtySelectionVisual();
    bool CanPaintPartial(float paintScale, int width, int height) const;
    void ConfigurePageBars();
    void PullPageScroll();
    bool PageBarDriving(const Scrollbar& bar) const;
    Scrollbar* PageScrollbarAt(float x, float y);
    const Scrollbar* PageScrollbarAt(float x, float y) const;

    std::vector<SceneItem> items_;
    Button* hoveredButton_ = nullptr;
    Input* hoveredInput_ = nullptr;
    Scrollbar* hoveredScrollbar_ = nullptr;
    Button* pressedButton_ = nullptr;
    Input* focusedInput_ = nullptr;
    Scrollbar* draggingScrollbar_ = nullptr;
    Scrollbar* focusedScrollbar_ = nullptr;
    Container* panningContainer_ = nullptr;
    float panX_ = 0.0f;
    float panY_ = 0.0f;

    Color background_ = Color::FromHex("#F7F3EB");
    float padding_ = 28.0f;
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    float appZoom_ = 1.0f;
    float lastPaintScale_ = 0.0f;
    float selectX_ = 0.0f;
    float selectY_ = 0.0f;
    int surfaceWidth_ = 1;
    int surfaceHeight_ = 1;
    float maxScrollX_ = 0.0f;
    float maxScrollY_ = 0.0f;
    Scrollbar pageVertical_;
    Scrollbar pageHorizontal_;
    bool pageBarsReady_ = false;
    IPlatform* platform_ = nullptr;
    FontEngine* fonts_ = nullptr;
    FrameBuffer frame_;
    bool hasRenderContext_ = false;
    bool needsRender_ = false;
    bool fullDirty_ = true;
    bool frameReady_ = false;
    bool damageOpen_ = false;
    float dirtyLeft_ = 0.0f;
    float dirtyTop_ = 0.0f;
    float dirtyRight_ = 0.0f;
    float dirtyBottom_ = 0.0f;
    float paintedScrollX_ = 0.0f;
    float paintedScrollY_ = 0.0f;
    float paintedScale_ = 0.0f;
    int paintedWidth_ = 0;
    int paintedHeight_ = 0;
    float paintedVerticalReserve_ = 0.0f;
    float paintedHorizontalReserve_ = 0.0f;
    std::string foundation_ = "RoboFoundation";
    std::string lightFoundation_;
    std::string darkFoundation_;
    Theme theme_ = Theme::Light;
};

}  // namespace Robo
