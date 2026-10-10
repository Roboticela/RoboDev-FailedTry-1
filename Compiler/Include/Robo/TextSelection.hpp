#pragma once

#include "Selectable.hpp"

#include <cstdint>
#include <string>

namespace Robo {

// Glyph map rebuilt every frame for components that opted into selection.
// Pointer hits, the highlight, and the clipboard all read this same list.
class TextSelection {
public:
    static void BeginFrame();
    static void EndFrame();

    static void Push(const void* host, SelectableMode mode);
    static void Pop();
    static bool IsOpen();

    static int AddCodepoint(char32_t codepoint, float x, float y, float width, float height);

    static bool Covers(int index);
    static bool IsCaret(int index);

    static int Hit(float x, float y);
    static void BeginDrag(int index);
    static void MoveFocus(int index);
    static void EndDrag();
    static void Clear();
    static bool IsDragging();

    static bool SelectAll();
    static std::string SelectedText();
    static bool HasSelection();
};

struct SelectableScope {
    SelectableScope(const void* host, SelectableMode mode) { TextSelection::Push(host, mode); }
    ~SelectableScope() { TextSelection::Pop(); }
};

}  // namespace Robo
