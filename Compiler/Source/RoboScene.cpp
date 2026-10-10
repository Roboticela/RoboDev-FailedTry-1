#include "Robo/RoboScene.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/FlexboxPainter.hpp"
#include "Robo/GridPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/PositionPainter.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cstdint>

namespace Robo {

namespace {

void Downsample2x(const FrameBuffer& source, FrameBuffer& destination) {
    const int width = destination.GetWidth();
    const int height = destination.GetHeight();
    const std::uint8_t* src = source.GetPixels();
    std::uint8_t* dst = destination.GetPixels();
    const int srcWidth = source.GetWidth();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int sx = x * 2;
            const int sy = y * 2;
            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int a = 0;
            for (int oy = 0; oy < 2; ++oy) {
                for (int ox = 0; ox < 2; ++ox) {
                    const std::size_t index =
                        static_cast<std::size_t>(((sy + oy) * srcWidth + (sx + ox)) * 4);
                    r += src[index + 0];
                    g += src[index + 1];
                    b += src[index + 2];
                    a += src[index + 3];
                }
            }
            const std::size_t out = static_cast<std::size_t>((y * width + x) * 4);
            dst[out + 0] = static_cast<std::uint8_t>((r + 2) / 4);
            dst[out + 1] = static_cast<std::uint8_t>((g + 2) / 4);
            dst[out + 2] = static_cast<std::uint8_t>((b + 2) / 4);
            dst[out + 3] = static_cast<std::uint8_t>((a + 2) / 4);
        }
    }
}

void ScaleBounds(ButtonBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleBounds(InputBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleBounds(ScrollbarBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleScrollbar(Scrollbar& scrollbar, float inv) {
    ScrollbarBounds bounds = scrollbar.GetBounds();
    ScaleBounds(bounds, inv);
    scrollbar.SetBounds(bounds);
    scrollbar.SetPixelScale(scrollbar.GetPixelScale() * inv);
}

void ScaleBounds(FlexboxBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleFlexboxBounds(Flexbox& flexbox, float inv);
void ScaleGridBounds(Grid& grid, float inv);

void ScaleBounds(GridBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleFlexboxBounds(Flexbox& flexbox, float inv) {
    FlexboxBounds fb = flexbox.GetBounds();
    ScaleBounds(fb, inv);
    flexbox.SetBounds(fb);

    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Button) {
            ButtonBounds bb = item.button.GetBounds();
            ScaleBounds(bb, inv);
            item.button.SetBounds(bb);
        } else if (item.kind == FlexItemKind::Input) {
            InputBounds ib = item.input.GetBounds();
            ScaleBounds(ib, inv);
            item.input.SetBounds(ib);
        } else if (item.kind == FlexItemKind::Scrollbar) {
            ScaleScrollbar(item.scrollbar, inv);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        }
    }
}

void ScaleGridBounds(Grid& grid, float inv) {
    GridBounds gb = grid.GetBounds();
    ScaleBounds(gb, inv);
    grid.SetBounds(gb);

    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Button) {
            ButtonBounds bb = item.button.GetBounds();
            ScaleBounds(bb, inv);
            item.button.SetBounds(bb);
        } else if (item.kind == GridItemKind::Input) {
            InputBounds ib = item.input.GetBounds();
            ScaleBounds(ib, inv);
            item.input.SetBounds(ib);
        } else if (item.kind == GridItemKind::Scrollbar) {
            ScaleScrollbar(item.scrollbar, inv);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            ScaleGridBounds(*item.grid, inv);
        }
    }
}

Button* FindButtonInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* FindInputInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* FindScrollbarInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* FindButtonInFlexboxConst(const Flexbox& flexbox, float x, float y) {
    const std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* FindInputInFlexboxConst(const Flexbox& flexbox, float x, float y) {
    const std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* FindScrollbarInFlexboxConst(const Flexbox& flexbox, float x, float y) {
    const std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Button* FindButtonInGrid(Grid& grid, float x, float y);
Input* FindInputInGrid(Grid& grid, float x, float y);
Scrollbar* FindScrollbarInGrid(Grid& grid, float x, float y);
const Button* FindButtonInGridConst(const Grid& grid, float x, float y);
const Input* FindInputInGridConst(const Grid& grid, float x, float y);
const Scrollbar* FindScrollbarInGridConst(const Grid& grid, float x, float y);

Button* FindButtonInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Button* found = FindButtonInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* FindInputInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Input* found = FindInputInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* FindScrollbarInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Scrollbar* found = FindScrollbarInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* FindButtonInGridConst(const Grid& grid, float x, float y) {
    const std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            const Button* found = FindButtonInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* FindInputInGridConst(const Grid& grid, float x, float y) {
    const std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            const Input* found = FindInputInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* FindScrollbarInGridConst(const Grid& grid, float x, float y) {
    const std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            const Scrollbar* found = FindScrollbarInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

void ScalePositionBounds(Position& position, float inv) {
    InputBounds pb = position.GetBounds();
    ScaleBounds(pb, inv);
    position.SetBounds(pb);

    for (PositionItem& item : position.GetItems()) {
        ScaleBounds(item.computedBounds, inv);
        if (item.kind == PositionItemKind::Button) {
            ButtonBounds bb = item.button.GetBounds();
            ScaleBounds(bb, inv);
            item.button.SetBounds(bb);
        } else if (item.kind == PositionItemKind::Input) {
            InputBounds ib = item.input.GetBounds();
            ScaleBounds(ib, inv);
            item.input.SetBounds(ib);
        } else if (item.kind == PositionItemKind::Scrollbar) {
            ScaleScrollbar(item.scrollbar, inv);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            ScaleGridBounds(*item.grid, inv);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            ScalePositionBounds(*item.position, inv);
        }
    }
}

Button* FindButtonInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Button* found = FindButtonInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Button* found = FindButtonInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* FindInputInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Input* found = FindInputInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Input* found = FindInputInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* FindScrollbarInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Scrollbar* found = FindScrollbarInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Scrollbar* found = FindScrollbarInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* FindButtonInPositionConst(const Position& position, float x, float y) {
    const std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        const PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            const Button* found = FindButtonInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            const Button* found = FindButtonInPositionConst(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* FindInputInPositionConst(const Position& position, float x, float y) {
    const std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        const PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            const Input* found = FindInputInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            const Input* found = FindInputInPositionConst(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* FindScrollbarInPositionConst(const Position& position, float x, float y) {
    const std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        const PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            const Scrollbar* found = FindScrollbarInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            const Scrollbar* found = FindScrollbarInPositionConst(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

}  // namespace

void RoboScene::Clear() {
    items_.clear();
    hoveredButton_ = nullptr;
    hoveredInput_ = nullptr;
    hoveredScrollbar_ = nullptr;
    pressedButton_ = nullptr;
    focusedInput_ = nullptr;
    draggingScrollbar_ = nullptr;
}

void RoboScene::AddText(Text text) {
    SceneItem item;
    item.kind = SceneItemKind::Text;
    item.text = std::move(text);
    items_.push_back(std::move(item));
}

void RoboScene::AddButton(Button button) {
    SceneItem item;
    item.kind = SceneItemKind::Button;
    item.button = std::move(button);
    items_.push_back(std::move(item));
}

void RoboScene::AddInput(Input input) {
    SceneItem item;
    item.kind = SceneItemKind::Input;
    item.input = std::move(input);
    items_.push_back(std::move(item));
}

void RoboScene::AddScrollbar(Scrollbar scrollbar) {
    SceneItem item;
    item.kind = SceneItemKind::Scrollbar;
    item.scrollbar = std::move(scrollbar);
    items_.push_back(std::move(item));
}

void RoboScene::AddFlexbox(Flexbox flexbox) {
    SceneItem item;
    item.kind = SceneItemKind::Flexbox;
    item.flexbox = std::move(flexbox);
    items_.push_back(std::move(item));
}

void RoboScene::AddGrid(Grid grid) {
    SceneItem item;
    item.kind = SceneItemKind::Grid;
    item.grid = std::move(grid);
    items_.push_back(std::move(item));
}

void RoboScene::AddPosition(Position position) {
    SceneItem item;
    item.kind = SceneItemKind::Position;
    item.position = std::move(position);
    items_.push_back(std::move(item));
}

bool RoboScene::RerenderIfPossible() {
    if (!hasRenderContext_ || !platform_ || !fonts_) {
        return false;
    }
    Render(*platform_, *fonts_, background_, padding_);
    return true;
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) {
    platform_ = &platform;
    fonts_ = &fonts;
    background_ = background;
    padding_ = padding;
    hasRenderContext_ = true;

    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    const float pixelScale = std::max(0.5f, platform.GetPixelScale());
    int superSample = 2;
    const long long hiResBytes =
        static_cast<long long>(width) * static_cast<long long>(height) * 4LL * 4LL;
    if (hiResBytes > 96LL * 1024LL * 1024LL) {
        superSample = 1;
    }
    const float paintScale = pixelScale * static_cast<float>(superSample);

    FrameBuffer hiRes(width * superSample, height * superSample);
    hiRes.Clear(background);

    const float pad = padding * paintScale;
    const float contentWidth = static_cast<float>(width * superSample) - pad * 2.0f;
    float cursorY = pad;

    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Text) {
            cursorY += TextPainter::Paint(hiRes, fonts, item.text, pad, cursorY, contentWidth, paintScale);
        } else if (item.kind == SceneItemKind::Button) {
            const ButtonPaintResult painted =
                ButtonPainter::Paint(hiRes, fonts, item.button, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                ButtonBounds bounds = item.button.GetBounds();
                ScaleBounds(bounds, 1.0f / static_cast<float>(superSample));
                item.button.SetBounds(bounds);
            }
        } else if (item.kind == SceneItemKind::Input) {
            if (&item.input == focusedInput_) {
                item.input.SetInteraction(InputInteraction::Focused);
            } else if (&item.input == hoveredInput_ && item.input.GetInteraction() != InputInteraction::Focused) {
                item.input.SetInteraction(InputInteraction::Hovered);
            } else if (&item.input != focusedInput_) {
                item.input.SetInteraction(InputInteraction::Normal);
            }

            const InputPaintResult painted =
                InputPainter::Paint(hiRes, fonts, item.input, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                InputBounds bounds = item.input.GetBounds();
                ScaleBounds(bounds, 1.0f / static_cast<float>(superSample));
                item.input.SetBounds(bounds);
            }
        } else if (item.kind == SceneItemKind::Scrollbar) {
            if (&item.scrollbar == draggingScrollbar_) {
                item.scrollbar.SetInteraction(ScrollbarInteraction::Dragging);
            } else if (&item.scrollbar == hoveredScrollbar_) {
                item.scrollbar.SetInteraction(ScrollbarInteraction::Hovered);
            } else {
                item.scrollbar.SetInteraction(ScrollbarInteraction::Normal);
            }

            const ScrollbarPaintResult painted =
                ScrollbarPainter::Paint(hiRes, item.scrollbar, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                ScaleScrollbar(item.scrollbar, 1.0f / static_cast<float>(superSample));
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const FlexboxPaintResult painted =
                FlexboxPainter::Paint(hiRes, fonts, item.flexbox, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                ScaleFlexboxBounds(item.flexbox, 1.0f / static_cast<float>(superSample));
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const GridPaintResult painted =
                GridPainter::Paint(hiRes, fonts, item.grid, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                ScaleGridBounds(item.grid, 1.0f / static_cast<float>(superSample));
            }
        } else if (item.kind == SceneItemKind::Position) {
            const PositionPaintResult painted =
                PositionPainter::Paint(hiRes, fonts, item.position, pad, cursorY, contentWidth, paintScale);
            cursorY += painted.advanceY;
            if (superSample > 1) {
                ScalePositionBounds(item.position, 1.0f / static_cast<float>(superSample));
            }
        }
    }

    if (superSample > 1) {
        FrameBuffer output(width, height);
        Downsample2x(hiRes, output);
        platform.Present(output);
    } else {
        platform.Present(hiRes);
    }
}

Button* RoboScene::FindButtonAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            Button* found = FindButtonInFlexbox(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            Button* found = FindButtonInGrid(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            Button* found = FindButtonInPosition(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* RoboScene::FindInputAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            Input* found = FindInputInFlexbox(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            Input* found = FindInputInGrid(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            Input* found = FindInputInPosition(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* RoboScene::FindScrollbarAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            Scrollbar* found = FindScrollbarInGrid(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            Scrollbar* found = FindScrollbarInPosition(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* RoboScene::FindButtonAt(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const Button* found = FindButtonInFlexboxConst(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const Button* found = FindButtonInGridConst(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            const Button* found = FindButtonInPositionConst(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* RoboScene::FindInputAt(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const Input* found = FindInputInFlexboxConst(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const Input* found = FindInputInGridConst(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            const Input* found = FindInputInPositionConst(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* RoboScene::FindScrollbarAt(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const Scrollbar* found = FindScrollbarInGridConst(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            const Scrollbar* found = FindScrollbarInPositionConst(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

HitKind RoboScene::HitTestKind(float x, float y) const {
    if (FindScrollbarAt(x, y)) {
        return HitKind::Scrollbar;
    }
    if (FindButtonAt(x, y)) {
        return HitKind::Button;
    }
    if (FindInputAt(x, y)) {
        return HitKind::Input;
    }
    return HitKind::None;
}

bool RoboScene::HitTest(float x, float y) const {
    return HitTestKind(x, y) != HitKind::None;
}

bool RoboScene::SetHoveredButton(Button* button) {
    if (hoveredButton_ == button) {
        return false;
    }
    if (hoveredButton_ && hoveredButton_ != pressedButton_) {
        hoveredButton_->SetInteraction(ButtonInteraction::Normal);
    }
    hoveredButton_ = button;
    if (hoveredButton_ && hoveredButton_ != pressedButton_) {
        hoveredButton_->SetInteraction(ButtonInteraction::Hovered);
    }
    return true;
}

bool RoboScene::SetHoveredInput(Input* input) {
    if (hoveredInput_ == input) {
        return false;
    }
    if (hoveredInput_ && hoveredInput_ != focusedInput_) {
        hoveredInput_->SetInteraction(InputInteraction::Normal);
    }
    hoveredInput_ = input;
    if (hoveredInput_ && hoveredInput_ != focusedInput_) {
        hoveredInput_->SetInteraction(InputInteraction::Hovered);
    }
    return true;
}

bool RoboScene::SetHoveredScrollbar(Scrollbar* scrollbar) {
    if (hoveredScrollbar_ == scrollbar) {
        return false;
    }
    if (hoveredScrollbar_ && hoveredScrollbar_ != draggingScrollbar_) {
        hoveredScrollbar_->SetInteraction(ScrollbarInteraction::Normal);
    }
    hoveredScrollbar_ = scrollbar;
    if (hoveredScrollbar_ && hoveredScrollbar_ != draggingScrollbar_) {
        hoveredScrollbar_->SetInteraction(ScrollbarInteraction::Hovered);
    }
    return true;
}

bool RoboScene::SetPressedButton(Button* button) {
    if (pressedButton_ == button) {
        return false;
    }
    if (pressedButton_) {
        const ButtonInteraction next =
            (pressedButton_ == hoveredButton_) ? ButtonInteraction::Hovered : ButtonInteraction::Normal;
        pressedButton_->SetInteraction(next);
    }
    pressedButton_ = button;
    if (pressedButton_) {
        pressedButton_->SetInteraction(ButtonInteraction::Pressed);
    }
    return true;
}

bool RoboScene::SetFocusedInput(Input* input) {
    if (focusedInput_ == input) {
        return false;
    }
    if (focusedInput_) {
        const InputInteraction next =
            (focusedInput_ == hoveredInput_) ? InputInteraction::Hovered : InputInteraction::Normal;
        focusedInput_->SetInteraction(next);
    }
    focusedInput_ = input;
    if (focusedInput_) {
        focusedInput_->SetInteraction(InputInteraction::Focused);
        focusedInput_->SetCaretIndex(focusedInput_->GetValue().size());
    }
    return true;
}

bool RoboScene::PointerMove(float x, float y) {
    if (draggingScrollbar_) {
        draggingScrollbar_->UpdateDrag(x, y);
        RerenderIfPossible();
        return true;
    }

    if (pressedButton_) {
        const bool inside = FindButtonAt(x, y) == pressedButton_;
        const ButtonInteraction next = inside ? ButtonInteraction::Pressed : ButtonInteraction::Hovered;
        if (pressedButton_->GetInteraction() != next) {
            pressedButton_->SetInteraction(next);
            RerenderIfPossible();
            return true;
        }
        return false;
    }

    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    Button* button = scrollbar ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || button) ? nullptr : FindInputAt(x, y);

    bool changed = SetHoveredScrollbar(scrollbar);
    changed = SetHoveredButton(button) || changed;
    changed = SetHoveredInput(input) || changed;
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::PointerDown(float x, float y) {
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    Button* button = scrollbar ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || button) ? nullptr : FindInputAt(x, y);

    bool changed = false;
    if (scrollbar) {
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        scrollbar->BeginDrag(x, y);
        draggingScrollbar_ = scrollbar;
        changed = SetHoveredScrollbar(scrollbar) || changed;
        changed = true;
    } else if (button) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(button) || changed;
        changed = SetHoveredButton(button) || changed;
        changed = SetHoveredInput(nullptr) || changed;
    } else if (input) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedInput(input) || changed;
        changed = SetHoveredInput(input) || changed;
        changed = SetHoveredButton(nullptr) || changed;
    } else {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
    }

    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::PointerUp(float x, float y) {
    bool changed = false;
    if (draggingScrollbar_) {
        draggingScrollbar_->EndDrag();
        hoveredScrollbar_ = nullptr;
        draggingScrollbar_ = nullptr;
        changed = true;
    }

    changed = SetPressedButton(nullptr) || changed;
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    Button* button = scrollbar ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || button) ? nullptr : FindInputAt(x, y);

    changed = SetHoveredScrollbar(scrollbar) || changed;
    changed = SetHoveredButton(button) || changed;
    changed = SetHoveredInput(input) || changed;
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::PointerLeave() {
    bool changed = false;
    if (pressedButton_) {
        changed = SetPressedButton(nullptr) || changed;
    }
    if (hoveredButton_) {
        changed = SetHoveredButton(nullptr) || changed;
    }
    if (hoveredInput_) {
        changed = SetHoveredInput(nullptr) || changed;
    }
    if (hoveredScrollbar_) {
        changed = SetHoveredScrollbar(nullptr) || changed;
    }
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

bool RoboScene::HasFocusedInput() const {
    return focusedInput_ != nullptr;
}

bool RoboScene::KeyInsert(const std::string& utf8) {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->InsertText(utf8)) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyBackspace() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteBackward()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyDelete() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteForward()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyLeft() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->MoveCaretLeft()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyRight() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->MoveCaretRight()) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::KeyEscape() {
    if (!focusedInput_) {
        return false;
    }
    const bool changed = SetFocusedInput(nullptr);
    if (changed) {
        RerenderIfPossible();
    }
    return changed;
}

}  // namespace Robo
