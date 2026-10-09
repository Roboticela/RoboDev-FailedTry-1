#pragma once

#include "Button.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "IPlatform.hpp"
#include "Text.hpp"

#include <vector>

namespace Robo {

enum class SceneItemKind {
    Text,
    Button
};

struct SceneItem {
    SceneItemKind kind = SceneItemKind::Text;
    Text text;
    Button button;
};

class RoboScene {
public:
    void Clear();
    void AddText(Text text);
    void AddButton(Button button);
    void Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) const;

private:
    std::vector<SceneItem> items_;
};

}  // namespace Robo
