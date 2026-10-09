#include "WebPlatform.hpp"

#include <algorithm>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <vector>

namespace Robo {

namespace {

EM_JS(void, JsPresentRgba, (int width, int height, float pixelScale, const uint8_t* pixels), {
    const canvas = document.getElementById("robo-preview-canvas");
    if (!canvas) {
        return;
    }
    if (canvas.width !== width || canvas.height !== height) {
        canvas.width = width;
        canvas.height = height;
    }
    const scale = Math.max(0.01, pixelScale || 1);
    const cssWidth = Math.max(1, Math.round(width / scale));
    const cssHeight = Math.max(1, Math.round(height / scale));
    canvas.style.width = cssWidth + "px";
    canvas.style.height = cssHeight + "px";
    const context = canvas.getContext("2d", { alpha: false, desynchronized: true });
    context.setTransform(1, 0, 0, 1, 0, 0);
    context.imageSmoothingEnabled = false;
    const imageData = context.createImageData(width, height);
    imageData.data.set(HEAPU8.subarray(pixels, pixels + width * height * 4));
    context.putImageData(imageData, 0, 0);
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

std::vector<std::uint8_t> WebPlatform::LoadAsset(const std::string& relativePath) const {
    const std::string path = std::string("/") + relativePath;
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

}  // namespace Robo
