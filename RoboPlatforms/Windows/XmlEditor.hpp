#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

namespace Robo {

class XmlEditor {
public:
    static bool RegisterClassOnce(HINSTANCE instance);
    static HWND Create(HWND parent, HINSTANCE instance, int x, int y, int width, int height);

    static std::string GetTextUtf8(HWND hwnd);
    static void SetTextUtf8(HWND hwnd, const std::string& text);
};

}  // namespace Robo
