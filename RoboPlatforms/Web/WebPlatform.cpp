#include "WebPlatform.hpp"

#include <algorithm>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <vector>

namespace Robo {

namespace {

EM_JS(void, JsPresentRgba, (int width, int height, float pixelScale, const uint8_t* pixels), {
    const canvas = document.getElementById("robo-canvas");
    if (!canvas) {
        return;
    }
    if (canvas.width !== width || canvas.height !== height) {
        canvas.width = width;
        canvas.height = height;
    }
    canvas.style.position = "fixed";
    canvas.style.left = "0px";
    canvas.style.top = "0px";
    canvas.style.width = "100%";
    canvas.style.height = "100%";
    const context = canvas.getContext("2d", { alpha: false, desynchronized: true });
    context.setTransform(1, 0, 0, 1, 0, 0);
    context.imageSmoothingEnabled = false;
    const imageData = context.createImageData(width, height);
    imageData.data.set(HEAPU8.subarray(pixels, pixels + width * height * 4));
    context.putImageData(imageData, 0, 0);
});

EM_JS(void, JsCopyText, (const char* text), {
    const value = UTF8ToString(text);
    const area = document.createElement("textarea");
    area.value = value;
    area.setAttribute("readonly", "");
    area.style.position = "fixed";
    area.style.opacity = "0";
    document.body.appendChild(area);
    area.focus();
    area.select();
    document.execCommand("copy");
    area.remove();
});

}  // namespace

int WebPlatform::GetSurfaceWidth() const {
    return width_;
}

int WebPlatform::GetSurfaceHeight() const {
    return height_;
}

float WebPlatform::GetPixelScale() const {
    return pixelScale_;
}

void WebPlatform::SetSurfaceSize(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
}

void WebPlatform::SetPixelScale(float scale) {
    pixelScale_ = std::max(0.5f, scale);
}

void WebPlatform::Present(const FrameBuffer& buffer) {
    JsPresentRgba(buffer.GetWidth(), buffer.GetHeight(), pixelScale_, buffer.GetPixels());
}

void WebPlatform::CopyText(const std::string& text) {
    JsCopyText(text.c_str());
}

std::vector<std::uint8_t> WebPlatform::LoadAsset(const std::string& relativePath) const {
    const std::string path = std::string("/") + relativePath;
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

}  // namespace Robo
