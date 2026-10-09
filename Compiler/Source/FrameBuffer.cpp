#include "Robo/FrameBuffer.hpp"

#include <algorithm>

namespace Robo {

FrameBuffer::FrameBuffer(int width, int height) {
    Resize(width, height);
}

void FrameBuffer::Resize(int width, int height) {
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    pixels_.assign(static_cast<std::size_t>(width_ * height_ * 4), 0);
}

void FrameBuffer::Clear(const Color& color) {
    for (int i = 0; i < width_ * height_; ++i) {
        const std::size_t index = static_cast<std::size_t>(i) * 4;
        pixels_[index + 0] = color.red;
        pixels_[index + 1] = color.green;
        pixels_[index + 2] = color.blue;
        pixels_[index + 3] = color.alpha;
    }
}

int FrameBuffer::GetWidth() const {
    return width_;
}

int FrameBuffer::GetHeight() const {
    return height_;
}

const std::uint8_t* FrameBuffer::GetPixels() const {
    return pixels_.data();
}

std::uint8_t* FrameBuffer::GetPixels() {
    return pixels_.data();
}

void FrameBuffer::BlendPixel(int x, int y, const Color& color, float opacity) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }

    const float sourceAlpha = (static_cast<float>(color.alpha) / 255.0f) * opacity;
    if (sourceAlpha <= 0.0f) {
        return;
    }

    const std::size_t index = static_cast<std::size_t>((y * width_ + x) * 4);
    const float destR = pixels_[index + 0] / 255.0f;
    const float destG = pixels_[index + 1] / 255.0f;
    const float destB = pixels_[index + 2] / 255.0f;
    const float destA = pixels_[index + 3] / 255.0f;

    const float srcR = color.red / 255.0f;
    const float srcG = color.green / 255.0f;
    const float srcB = color.blue / 255.0f;

    const float outA = sourceAlpha + destA * (1.0f - sourceAlpha);
    float outR = 0.0f;
    float outG = 0.0f;
    float outB = 0.0f;
    if (outA > 0.0f) {
        outR = (srcR * sourceAlpha + destR * destA * (1.0f - sourceAlpha)) / outA;
        outG = (srcG * sourceAlpha + destG * destA * (1.0f - sourceAlpha)) / outA;
        outB = (srcB * sourceAlpha + destB * destA * (1.0f - sourceAlpha)) / outA;
    }

    pixels_[index + 0] = static_cast<std::uint8_t>(std::min(255.0f, outR * 255.0f + 0.5f));
    pixels_[index + 1] = static_cast<std::uint8_t>(std::min(255.0f, outG * 255.0f + 0.5f));
    pixels_[index + 2] = static_cast<std::uint8_t>(std::min(255.0f, outB * 255.0f + 0.5f));
    pixels_[index + 3] = static_cast<std::uint8_t>(std::min(255.0f, outA * 255.0f + 0.5f));
}

void FrameBuffer::FillRect(int x, int y, int width, int height, const Color& color, float opacity) {
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(width_, x + width);
    const int y1 = std::min(height_, y + height);
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            BlendPixel(px, py, color, opacity);
        }
    }
}

}  // namespace Robo
