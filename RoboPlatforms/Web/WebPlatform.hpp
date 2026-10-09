#pragma once

#include "Robo/IPlatform.hpp"

namespace Robo {

class WebPlatform : public IPlatform {
public:
    int GetSurfaceWidth() const override;
    int GetSurfaceHeight() const override;
    void SetSurfaceSize(int width, int height) override;
    void Present(const FrameBuffer& buffer) override;
    std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const override;

private:
    int width_ = 800;
    int height_ = 600;
};

}  // namespace Robo
