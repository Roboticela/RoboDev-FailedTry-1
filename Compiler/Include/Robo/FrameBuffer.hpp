#pragma once

#include "Color.hpp"

#include <cstdint>
#include <vector>

namespace Robo {

class FrameBuffer {
public:
    FrameBuffer() = default;
    FrameBuffer(int width, int height);

    void Resize(int width, int height);
    void Clear(const Color& color);

    int GetWidth() const;
    int GetHeight() const;
    const std::uint8_t* GetPixels() const;
    std::uint8_t* GetPixels();

    void BlendPixel(int x, int y, const Color& color, float opacity = 1.0f);
    void FillRect(int x, int y, int width, int height, const Color& color, float opacity = 1.0f);

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<std::uint8_t> pixels_;
};

}  // namespace Robo
