#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "WindowsPlatform.hpp"
#include "XmlEditor.hpp"
#include "XmlRuntime.hpp"
#include "Robo/FontEngine.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr wchar_t MAIN_CLASS[] = L"RoboIdeMain";
constexpr int ID_EDITOR = 1001;
constexpr int ID_PREVIEW = 1002;
constexpr int SPLITTER_WIDTH = 6;
constexpr UINT_PTR RENDER_TIMER = 1;

struct AppState {
    HWND editor = nullptr;
    HWND preview = nullptr;
    HWND status = nullptr;
    int splitX = 0;
    bool dragging = false;
    std::unique_ptr<Robo::WindowsPlatform> platform;
    std::unique_ptr<Robo::FontEngine> fonts;
    std::unique_ptr<Robo::XmlRuntime> runtime;
    std::wstring assetRoot;
    std::string pendingXml;
    bool renderQueued = false;
};

AppState* gApp = nullptr;

std::wstring GetExecutableDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring full(path);
    const std::size_t slash = full.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return full.substr(0, slash);
}

std::wstring FindAssetRoot(const std::wstring& exeDir) {
    const std::wstring candidates[] = {
        exeDir + L"\\..\\..\\..",
        exeDir + L"\\..\\..",
        exeDir + L"\\..",
        exeDir,
    };
    for (const std::wstring& candidate : candidates) {
        const std::wstring probe = candidate + L"\\Assets\\Fonts\\Inter-Regular.ttf";
        if (GetFileAttributesW(probe.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return candidate;
        }
    }
    return exeDir + L"\\..\\..\\..";
}

std::string LoadFileUtf8(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void SetStatus(const std::wstring& text) {
    if (gApp && gApp->status) {
        SetWindowTextW(gApp->status, text.c_str());
    }
}

void LayoutChildren(HWND hwnd) {
    RECT rect{};
    GetClientRect(hwnd, &rect);
    const int header = 48;
    const int title = 36;
    if (gApp->splitX <= 0) {
        gApp->splitX = rect.right * 48 / 100;
    }
    gApp->splitX = std::max(220, std::min(gApp->splitX, static_cast<int>(rect.right) - 220));

    if (gApp->status) {
        MoveWindow(gApp->status, 16, 12, rect.right - 32, 24, TRUE);
    }

    const int bodyTop = header;
    const int bodyHeight = rect.bottom - bodyTop;
    const int editorWidth = gApp->splitX;
    const int previewLeft = gApp->splitX + SPLITTER_WIDTH;
    const int previewWidth = rect.right - previewLeft;

    MoveWindow(gApp->editor, 0, bodyTop + title, editorWidth, bodyHeight - title, TRUE);
    MoveWindow(gApp->preview, previewLeft, bodyTop + title, previewWidth, bodyHeight - title, TRUE);

    if (gApp->platform) {
        RECT previewRect{};
        GetClientRect(gApp->preview, &previewRect);
        gApp->platform->SetSurfaceSize(previewRect.right, previewRect.bottom);
    }
}

void RenderPreview() {
    if (!gApp || !gApp->runtime || !gApp->platform) {
        return;
    }
    RECT previewRect{};
    GetClientRect(gApp->preview, &previewRect);
    gApp->platform->SetSurfaceSize(std::max(1, static_cast<int>(previewRect.right)),
                                   std::max(1, static_cast<int>(previewRect.bottom)));

    const std::string xml = Robo::XmlEditor::GetTextUtf8(gApp->editor);
    const bool ok = gApp->runtime->RenderXml(xml);
    if (!ok) {
        SetStatus(L"XML error — check markup");
    } else {
        SetStatus(L"RoboIDE  ·  Windows native  ·  Live");
    }
}

void QueueRender(HWND hwnd) {
    if (!gApp) {
        return;
    }
    gApp->renderQueued = true;
    SetTimer(hwnd, RENDER_TIMER, 160, nullptr);
}

void MapPreviewPoint(HWND hwnd, LPARAM lParam, float& x, float& y) {
    RECT client{};
    GetClientRect(hwnd, &client);
    const int clientWidth = std::max(1, static_cast<int>(client.right));
    const int clientHeight = std::max(1, static_cast<int>(client.bottom));
    const int bufferWidth = gApp && gApp->platform ? std::max(1, gApp->platform->GetSurfaceWidth()) : clientWidth;
    const int bufferHeight = gApp && gApp->platform ? std::max(1, gApp->platform->GetSurfaceHeight()) : clientHeight;
    const int mouseX = static_cast<short>(LOWORD(lParam));
    const int mouseY = static_cast<short>(HIWORD(lParam));
    x = static_cast<float>(mouseX) * static_cast<float>(bufferWidth) / static_cast<float>(clientWidth);
    y = static_cast<float>(mouseY) * static_cast<float>(bufferHeight) / static_cast<float>(clientHeight);
}

LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC hdc = BeginPaint(hwnd, &paint);
            if (gApp && gApp->platform) {
                gApp->platform->Paint(hdc);
            }
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            if (gApp && gApp->platform) {
                gApp->platform->SetSurfaceSize(LOWORD(lParam), HIWORD(lParam));
                QueueRender(GetParent(hwnd));
            }
            return 0;
        case WM_MOUSEMOVE: {
            if (!gApp || !gApp->runtime) {
                return 0;
            }
            TRACKMOUSEEVENT track{};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            TrackMouseEvent(&track);

            float x = 0.0f;
            float y = 0.0f;
            MapPreviewPoint(hwnd, lParam, x, y);
            gApp->runtime->PointerMove(x, y);
            SetCursor(LoadCursor(nullptr, gApp->runtime->HitTest(x, y) ? IDC_HAND : IDC_ARROW));
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (!gApp || !gApp->runtime) {
                return 0;
            }
            SetCapture(hwnd);
            float x = 0.0f;
            float y = 0.0f;
            MapPreviewPoint(hwnd, lParam, x, y);
            gApp->runtime->PointerDown(x, y);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (!gApp || !gApp->runtime) {
                return 0;
            }
            float x = 0.0f;
            float y = 0.0f;
            MapPreviewPoint(hwnd, lParam, x, y);
            gApp->runtime->PointerUp(x, y);
            ReleaseCapture();
            SetCursor(LoadCursor(nullptr, gApp->runtime->HitTest(x, y) ? IDC_HAND : IDC_ARROW));
            return 0;
        }
        case WM_MOUSELEAVE:
            if (gApp && gApp->runtime) {
                gApp->runtime->PointerLeave();
            }
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            gApp = new AppState();
            HINSTANCE instance = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;

            gApp->status = CreateWindowExW(0,
                                           L"STATIC",
                                           L"RoboIDE  ·  Windows native",
                                           WS_CHILD | WS_VISIBLE,
                                           0,
                                           0,
                                           0,
                                           0,
                                           hwnd,
                                           nullptr,
                                           instance,
                                           nullptr);

            gApp->editor = Robo::XmlEditor::Create(hwnd, instance, 0, 0, 100, 100);

            WNDCLASSW previewClass{};
            previewClass.lpfnWndProc = PreviewProc;
            previewClass.hInstance = instance;
            previewClass.lpszClassName = L"RoboPreview";
            previewClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&previewClass);

            gApp->preview = CreateWindowExW(0,
                                            L"RoboPreview",
                                            L"",
                                            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                            0,
                                            0,
                                            100,
                                            100,
                                            hwnd,
                                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PREVIEW)),
                                            instance,
                                            nullptr);

            gApp->assetRoot = FindAssetRoot(GetExecutableDir());
            gApp->platform = std::make_unique<Robo::WindowsPlatform>(gApp->preview);
            gApp->platform->SetAssetRoot(gApp->assetRoot);
            gApp->fonts = std::make_unique<Robo::FontEngine>();
            gApp->runtime = std::make_unique<Robo::XmlRuntime>(*gApp->platform, *gApp->fonts);

            const std::string homeXml = LoadFileUtf8(gApp->assetRoot + L"\\Examples\\Home.xml");
            Robo::XmlEditor::SetTextUtf8(gApp->editor, homeXml.empty() ? "<Robo>\n  <Text>Hello</Text>\n</Robo>" : homeXml);
            LayoutChildren(hwnd);
            SetFocus(gApp->editor);
            QueueRender(hwnd);
            return 0;
        }
        case WM_SIZE:
            LayoutChildren(hwnd);
            QueueRender(hwnd);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EDITOR && HIWORD(wParam) == EN_CHANGE) {
                QueueRender(hwnd);
            }
            return 0;
        case WM_TIMER:
            if (wParam == RENDER_TIMER) {
                KillTimer(hwnd, RENDER_TIMER);
                if (gApp) {
                    gApp->renderQueued = false;
                }
                RenderPreview();
            }
            return 0;
        case WM_LBUTTONDOWN: {
            RECT rect{};
            GetClientRect(hwnd, &rect);
            const int x = LOWORD(lParam);
            if (x >= gApp->splitX && x <= gApp->splitX + SPLITTER_WIDTH) {
                gApp->dragging = true;
                SetCapture(hwnd);
            }
            return 0;
        }
        case WM_MOUSEMOVE:
            if (gApp && gApp->dragging) {
                gApp->splitX = LOWORD(lParam);
                LayoutChildren(hwnd);
                QueueRender(hwnd);
            }
            return 0;
        case WM_LBUTTONUP:
            if (gApp) {
                gApp->dragging = false;
            }
            ReleaseCapture();
            return 0;
        case WM_SETCURSOR: {
            POINT point{};
            GetCursorPos(&point);
            ScreenToClient(hwnd, &point);
            if (point.x >= gApp->splitX && point.x <= gApp->splitX + SPLITTER_WIDTH) {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC hdc = BeginPaint(hwnd, &paint);
            RECT rect{};
            GetClientRect(hwnd, &rect);

            HBRUSH headerBrush = CreateSolidBrush(RGB(20, 26, 34));
            RECT headerRect{0, 0, rect.right, 48};
            FillRect(hdc, &headerRect, headerBrush);
            DeleteObject(headerBrush);

            HBRUSH titleBrush = CreateSolidBrush(RGB(14, 17, 22));
            RECT leftTitle{0, 48, gApp->splitX, 48 + 36};
            RECT rightTitle{gApp->splitX + SPLITTER_WIDTH, 48, rect.right, 48 + 36};
            FillRect(hdc, &leftTitle, titleBrush);
            FillRect(hdc, &rightTitle, titleBrush);
            DeleteObject(titleBrush);

            HBRUSH splitterBrush = CreateSolidBrush(RGB(36, 48, 65));
            RECT splitterRect{gApp->splitX, 48, gApp->splitX + SPLITTER_WIDTH, rect.bottom};
            FillRect(hdc, &splitterRect, splitterBrush);
            DeleteObject(splitterBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(143, 161, 179));
            TextOutW(hdc, 12, 56, L"XML", 3);
            TextOutW(hdc, gApp->splitX + SPLITTER_WIDTH + 12, 56, L"PREVIEW", 7);

            SetTextColor(hdc, RGB(232, 238, 246));
            HFONT brandFont = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH, L"Segoe UI");
            HFONT old = static_cast<HFONT>(SelectObject(hdc, brandFont));
            TextOutW(hdc, 16, 12, L"RoboIDE", 7);
            SelectObject(hdc, old);
            DeleteObject(brandFont);

            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_DESTROY:
            KillTimer(hwnd, RENDER_TIMER);
            delete gApp;
            gApp = nullptr;
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int showCommand) {
    Robo::XmlEditor::RegisterClassOnce(instance);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = MainProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = MAIN_CLASS;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&windowClass);

    HWND hwnd = CreateWindowExW(0,
                                MAIN_CLASS,
                                L"RoboIDE",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                1400,
                                860,
                                nullptr,
                                nullptr,
                                instance,
                                nullptr);

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
