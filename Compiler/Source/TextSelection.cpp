#include "Robo/TextSelection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Robo {

namespace {

struct Glyph {
    const void* root = nullptr;
    char32_t codepoint = 0;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct Scope {
    const void* root = nullptr;
};

std::vector<Glyph> glyphs;
std::vector<Scope> scopes;
int anchor = -1;
int focus = -1;
bool dragging = false;

void AppendUtf8(std::string& out, char32_t codepoint) {
    const auto value = static_cast<std::uint32_t>(codepoint);
    if (value < 0x80) {
        out.push_back(static_cast<char>(value));
    } else if (value < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (value >> 6)));
        out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
    } else if (value < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (value >> 12)));
        out.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (value >> 18)));
        out.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
    }
}

int RangeStart() {
    return std::min(anchor, focus);
}

int RangeEnd() {
    return std::max(anchor, focus);
}

const void* AnchorRoot() {
    if (anchor < 0 || anchor >= static_cast<int>(glyphs.size())) {
        return nullptr;
    }
    return glyphs[static_cast<std::size_t>(anchor)].root;
}

bool SameRoot(int index) {
    const void* root = AnchorRoot();
    if (!root || index < 0 || index >= static_cast<int>(glyphs.size())) {
        return false;
    }
    return glyphs[static_cast<std::size_t>(index)].root == root;
}

}  // namespace

SelectableMode ParseSelectable(const std::string& value) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return SelectableMode::On;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return SelectableMode::Off;
    }
    return SelectableMode::Inherit;
}

void TextSelection::BeginFrame() {
    glyphs.clear();
    scopes.clear();
}

void TextSelection::EndFrame() {
    if (glyphs.empty()) {
        anchor = -1;
        focus = -1;
        dragging = false;
        return;
    }
    const int last = static_cast<int>(glyphs.size()) - 1;
    if (anchor > last) {
        anchor = last;
    }
    if (focus > last) {
        focus = last;
    }
}

void TextSelection::Push(const void* host, SelectableMode mode) {
    const void* parent = scopes.empty() ? nullptr : scopes.back().root;
    const void* root = parent;
    if (mode == SelectableMode::On) {
        root = host;
    } else if (mode == SelectableMode::Off) {
        root = nullptr;
    }
    scopes.push_back(Scope{root});
}

void TextSelection::Pop() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

bool TextSelection::IsOpen() {
    return !scopes.empty() && scopes.back().root != nullptr;
}

int TextSelection::AddCodepoint(char32_t codepoint, float x, float y, float width, float height) {
    if (!IsOpen() || width <= 0.0f || height <= 0.0f) {
        return -1;
    }
    Glyph glyph;
    glyph.root = scopes.back().root;
    glyph.codepoint = codepoint;
    glyph.x = x;
    glyph.y = y;
    glyph.width = std::max(1.0f, width);
    glyph.height = height;
    glyphs.push_back(glyph);
    return static_cast<int>(glyphs.size()) - 1;
}

bool TextSelection::Covers(int index) {
    if (anchor < 0 || focus < 0 || anchor == focus || !SameRoot(index)) {
        return false;
    }
    return index >= RangeStart() && index <= RangeEnd();
}

bool TextSelection::IsCaret(int index) {
    return anchor >= 0 && anchor == focus && index == anchor && SameRoot(index);
}

int TextSelection::Hit(float x, float y) {
    int found = -1;
    for (int i = 0; i < static_cast<int>(glyphs.size()); ++i) {
        const Glyph& glyph = glyphs[static_cast<std::size_t>(i)];
        if (x >= glyph.x && x < glyph.x + glyph.width && y >= glyph.y && y < glyph.y + glyph.height) {
            found = i;
        }
    }
    return found;
}

void TextSelection::BeginDrag(int index) {
    if (index < 0 || index >= static_cast<int>(glyphs.size())) {
        return;
    }
    anchor = index;
    focus = index;
    dragging = true;
}

void TextSelection::MoveFocus(int index) {
    if (!dragging || index < 0 || !SameRoot(index)) {
        return;
    }
    focus = index;
}

void TextSelection::EndDrag() {
    dragging = false;
}

void TextSelection::Clear() {
    anchor = -1;
    focus = -1;
    dragging = false;
}

bool TextSelection::IsDragging() {
    return dragging;
}

bool TextSelection::SelectAll() {
    const void* root = AnchorRoot();
    if (!root) {
        return false;
    }
    int first = -1;
    int last = -1;
    for (int i = 0; i < static_cast<int>(glyphs.size()); ++i) {
        if (glyphs[static_cast<std::size_t>(i)].root != root) {
            continue;
        }
        if (first < 0) {
            first = i;
        }
        last = i;
    }
    if (first < 0 || first == last) {
        return false;
    }
    anchor = first;
    focus = last;
    dragging = false;
    return true;
}

std::string TextSelection::SelectedText() {
    if (anchor < 0 || focus < 0 || anchor == focus) {
        return {};
    }
    const void* root = AnchorRoot();
    std::string text;
    float previousY = -100000.0f;
    float previousHeight = 0.0f;
    for (int i = RangeStart(); i <= RangeEnd(); ++i) {
        const Glyph& glyph = glyphs[static_cast<std::size_t>(i)];
        if (glyph.root != root) {
            continue;
        }
        if (!text.empty() && glyph.y > previousY + previousHeight * 0.5f) {
            text.push_back('\n');
        }
        AppendUtf8(text, glyph.codepoint);
        previousY = glyph.y;
        previousHeight = glyph.height;
    }
    return text;
}

bool TextSelection::HasSelection() {
    return anchor >= 0 && focus >= 0 && anchor != focus && AnchorRoot() != nullptr;
}

}  // namespace Robo
