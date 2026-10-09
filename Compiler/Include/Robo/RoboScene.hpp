#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "IPlatform.hpp"
#include "Text.hpp"

#include <vector>

namespace Robo {

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) const;

private:
    std::vector<Text> texts_;
};

}  // namespace Robo
