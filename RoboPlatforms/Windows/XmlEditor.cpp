#include "XmlEditor.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace Robo {

namespace {

constexpr wchar_t EDITOR_CLASS[] = L"RoboXmlEditor";
constexpr int LINE_HEIGHT = 22;
constexpr int CHAR_WIDTH = 9;
constexpr int PADDING_X = 12;
constexpr int PADDING_Y = 10;

enum class TokenKind {
    Text,
    Tag,
    Attribute,
    String,
    Comment,
    Bracket
};

struct Token {
    TokenKind kind = TokenKind::Text;
    std::wstring text;
};

struct EditorState {
    std::wstring content;
    std::size_t caret = 0;
    int scrollY = 0;
    bool selecting = false;
    std::size_t selAnchor = 0;
    HFONT font = nullptr;
};

EditorState* StateOf(HWND hwnd) {
    return reinterpret_cast<EditorState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

COLORREF ColorFor(TokenKind kind) {
    switch (kind) {
        case TokenKind::Tag:
            return RGB(122, 162, 247);
        case TokenKind::Attribute:
            return RGB(224, 175, 104);
        case TokenKind::String:
            return RGB(158, 206, 106);
        case TokenKind::Comment:
            return RGB(107, 124, 143);
        case TokenKind::Bracket:
            return RGB(192, 202, 245);
        case TokenKind::Text:
        default:
            return RGB(232, 238, 246);
    }
}

std::vector<Token> Tokenize(const std::wstring& source) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    while (i < source.size()) {
        if (i + 3 < source.size() && source.compare(i, 4, L"<!--") == 0) {
            const std::size_t end = source.find(L"-->", i + 4);
            const std::size_t stop = end == std::wstring::npos ? source.size() : end + 3;
            tokens.push_back({TokenKind::Comment, source.substr(i, stop - i)});
            i = stop;
            continue;
        }

        if (source[i] == L'<') {
            tokens.push_back({TokenKind::Bracket, L"<"});
            ++i;
            if (i < source.size() && source[i] == L'/') {
                tokens.push_back({TokenKind::Bracket, L"/"});
                ++i;
            }
            std::size_t start = i;
            while (i < source.size() && (iswalnum(source[i]) || source[i] == L'_' || source[i] == L'-' || source[i] == L':')) {
                ++i;
            }
            if (i > start) {
                tokens.push_back({TokenKind::Tag, source.substr(start, i - start)});
            }
            while (i < source.size() && source[i] != L'>') {
                while (i < source.size() && iswspace(source[i])) {
                    tokens.push_back({TokenKind::Text, std::wstring(1, source[i++])});
                }
                if (i < source.size() && source[i] == L'/') {
                    tokens.push_back({TokenKind::Bracket, L"/"});
                    ++i;
                    continue;
                }
                if (i >= source.size() || source[i] == L'>') {
                    break;
                }
                start = i;
                while (i < source.size() && source[i] != L'=' && source[i] != L'>' && !iswspace(source[i])) {
                    ++i;
                }
                if (i > start) {
                    tokens.push_back({TokenKind::Attribute, source.substr(start, i - start)});
                }
                if (i < source.size() && source[i] == L'=') {
                    tokens.push_back({TokenKind::Text, L"="});
                    ++i;
                }
                if (i < source.size() && (source[i] == L'"' || source[i] == L'\'')) {
                    const wchar_t quote = source[i++];
                    start = i - 1;
                    while (i < source.size() && source[i] != quote) {
                        ++i;
                    }
                    if (i < source.size()) {
                        ++i;
                    }
                    tokens.push_back({TokenKind::String, source.substr(start, i - start)});
                }
            }
            if (i < source.size() && source[i] == L'>') {
                tokens.push_back({TokenKind::Bracket, L">"});
                ++i;
            }
            continue;
        }

        const std::size_t start = i;
        while (i < source.size() && source[i] != L'<') {
            ++i;
        }
        tokens.push_back({TokenKind::Text, source.substr(start, i - start)});
    }
    return tokens;
}

std::string Narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string narrow(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), narrow.data(), size, nullptr, nullptr);
    return narrow;
}

std::wstring Widen(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), size);
    return wide;
}

int CaretLine(const EditorState& state) {
    int line = 0;
    for (std::size_t i = 0; i < state.caret && i < state.content.size(); ++i) {
        if (state.content[i] == L'\n') {
            ++line;
        }
    }
    return line;
}

int CaretColumn(const EditorState& state) {
    int column = 0;
    for (std::size_t i = state.caret; i > 0; --i) {
        if (state.content[i - 1] == L'\n') {
            break;
        }
        ++column;
    }
    return column;
}

void EnsureCaretVisible(HWND hwnd, EditorState& state) {
    RECT rect{};
    GetClientRect(hwnd, &rect);
    const int visibleLines = std::max(1, static_cast<int>((rect.bottom - PADDING_Y * 2) / LINE_HEIGHT));
    const int line = CaretLine(state);
    if (line < state.scrollY) {
        state.scrollY = line;
    } else if (line >= state.scrollY + visibleLines) {
        state.scrollY = line - visibleLines + 1;
    }
}

void NotifyChanged(HWND hwnd) {
    HWND parent = GetParent(hwnd);
    if (parent) {
        SendMessageW(parent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), reinterpret_cast<LPARAM>(hwnd));
    }
}

void PaintEditor(HWND hwnd, EditorState& state) {
    PAINTSTRUCT paint{};
    HDC hdc = BeginPaint(hwnd, &paint);
    RECT rect{};
    GetClientRect(hwnd, &rect);

    HBRUSH background = CreateSolidBrush(RGB(11, 15, 20));
    FillRect(hdc, &rect, background);
    DeleteObject(background);

    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, state.font));
    SetBkMode(hdc, TRANSPARENT);

    const std::vector<Token> tokens = Tokenize(state.content);
    int x = PADDING_X;
    int line = 0;
    int y = PADDING_Y - state.scrollY * LINE_HEIGHT;

    for (const Token& token : tokens) {
        SetTextColor(hdc, ColorFor(token.kind));
        for (wchar_t ch : token.text) {
            if (ch == L'\n') {
                x = PADDING_X;
                ++line;
                y = PADDING_Y + (line - state.scrollY) * LINE_HEIGHT;
                continue;
            }
            if (y > -LINE_HEIGHT && y < rect.bottom) {
                wchar_t glyph[2] = {ch, 0};
                TextOutW(hdc, x, y, glyph, 1);
            }
            x += CHAR_WIDTH;
        }
    }

    if (GetFocus() == hwnd) {
        const int caretX = PADDING_X + CaretColumn(state) * CHAR_WIDTH;
        const int caretY = PADDING_Y + (CaretLine(state) - state.scrollY) * LINE_HEIGHT;
        MoveToEx(hdc, caretX, caretY, nullptr);
        LineTo(hdc, caretX, caretY + LINE_HEIGHT - 2);
    }

    SelectObject(hdc, oldFont);
    EndPaint(hwnd, &paint);
}

void InsertText(EditorState& state, const std::wstring& text) {
    state.content.insert(state.caret, text);
    state.caret += text.size();
}

void DeleteSelectionOrChar(EditorState& state, bool forward) {
    if (state.content.empty()) {
        return;
    }
    if (!forward && state.caret > 0) {
        state.content.erase(state.caret - 1, 1);
        --state.caret;
        return;
    }
    if (forward && state.caret < state.content.size()) {
        state.content.erase(state.caret, 1);
    }
}

LRESULT CALLBACK EditorProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    EditorState* state = StateOf(hwnd);
    switch (message) {
        case WM_CREATE: {
            auto* created = new EditorState();
            created->font = CreateFontW(18,
                                        0,
                                        0,
                                        0,
                                        FW_NORMAL,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS,
                                        CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY,
                                        FIXED_PITCH | FF_MODERN,
                                        L"Consolas");
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created));
            return 0;
        }
        case WM_DESTROY:
            if (state) {
                if (state->font) {
                    DeleteObject(state->font);
                }
                delete state;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            }
            return 0;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT:
            if (state) {
                PaintEditor(hwnd, *state);
            }
            return 0;
        case WM_CHAR:
            if (!state) {
                return 0;
            }
            if (wParam == VK_BACK) {
                DeleteSelectionOrChar(*state, false);
            } else if (wParam == VK_RETURN) {
                InsertText(*state, L"\n");
            } else if (wParam >= 32) {
                wchar_t ch = static_cast<wchar_t>(wParam);
                InsertText(*state, std::wstring(1, ch));
            }
            EnsureCaretVisible(hwnd, *state);
            InvalidateRect(hwnd, nullptr, FALSE);
            NotifyChanged(hwnd);
            return 0;
        case WM_KEYDOWN:
            if (!state) {
                return 0;
            }
            if (wParam == VK_LEFT && state->caret > 0) {
                --state->caret;
            } else if (wParam == VK_RIGHT && state->caret < state->content.size()) {
                ++state->caret;
            } else if (wParam == VK_UP) {
                const int column = CaretColumn(*state);
                int lineStart = static_cast<int>(state->caret) - column;
                if (lineStart > 0) {
                    int prevLineEnd = lineStart - 1;
                    int prevLineStart = prevLineEnd;
                    while (prevLineStart > 0 && state->content[prevLineStart - 1] != L'\n') {
                        --prevLineStart;
                    }
                    state->caret = static_cast<std::size_t>(
                        std::min(prevLineStart + column, prevLineEnd));
                }
            } else if (wParam == VK_DOWN) {
                const int column = CaretColumn(*state);
                std::size_t lineEnd = state->caret;
                while (lineEnd < state->content.size() && state->content[lineEnd] != L'\n') {
                    ++lineEnd;
                }
                if (lineEnd < state->content.size()) {
                    std::size_t nextStart = lineEnd + 1;
                    std::size_t nextEnd = nextStart;
                    while (nextEnd < state->content.size() && state->content[nextEnd] != L'\n') {
                        ++nextEnd;
                    }
                    state->caret = std::min(nextStart + static_cast<std::size_t>(column), nextEnd);
                }
            } else if (wParam == VK_DELETE) {
                DeleteSelectionOrChar(*state, true);
                NotifyChanged(hwnd);
            } else if (wParam == VK_PRIOR) {
                state->scrollY = std::max(0, state->scrollY - 10);
            } else if (wParam == VK_NEXT) {
                state->scrollY += 10;
            }
            EnsureCaretVisible(hwnd, *state);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_MOUSEWHEEL:
            if (state) {
                const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                state->scrollY = std::max(0, state->scrollY - delta / WHEEL_DELTA * 3);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONDOWN:
            if (state) {
                SetFocus(hwnd);
                const int x = LOWORD(lParam);
                const int y = HIWORD(lParam);
                const int line = state->scrollY + std::max(0, (y - PADDING_Y) / LINE_HEIGHT);
                const int column = std::max(0, (x - PADDING_X) / CHAR_WIDTH);
                int currentLine = 0;
                std::size_t index = 0;
                while (index < state->content.size() && currentLine < line) {
                    if (state->content[index] == L'\n') {
                        ++currentLine;
                    }
                    ++index;
                }
                int currentColumn = 0;
                while (index < state->content.size() && state->content[index] != L'\n' && currentColumn < column) {
                    ++index;
                    ++currentColumn;
                }
                state->caret = index;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

}  // namespace

bool XmlEditor::RegisterClassOnce(HINSTANCE instance) {
    static bool registered = false;
    if (registered) {
        return true;
    }
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = EditorProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = EDITOR_CLASS;
    windowClass.hCursor = LoadCursor(nullptr, IDC_IBEAM);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    registered = RegisterClassW(&windowClass) != 0;
    return registered;
}

HWND XmlEditor::Create(HWND parent, HINSTANCE instance, int x, int y, int width, int height) {
    RegisterClassOnce(instance);
    return CreateWindowExW(0,
                           EDITOR_CLASS,
                           L"",
                           WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS,
                           x,
                           y,
                           width,
                           height,
                           parent,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(1001)),
                           instance,
                           nullptr);
}

std::string XmlEditor::GetTextUtf8(HWND hwnd) {
    EditorState* state = StateOf(hwnd);
    if (!state) {
        return {};
    }
    return Narrow(state->content);
}

void XmlEditor::SetTextUtf8(HWND hwnd, const std::string& text) {
    EditorState* state = StateOf(hwnd);
    if (!state) {
        return;
    }
    state->content = Widen(text);
    state->caret = state->content.size();
    state->scrollY = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
}

}  // namespace Robo
