#include "Robo/RoboScene.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/ContainerPainter.hpp"
#include "Robo/FlexboxPainter.hpp"
#include "Robo/GridPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/PositionPainter.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

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
void ScalePositionBounds(Position& position, float inv);
void ScaleContainerBounds(Container& container, float inv);

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
        } else if (item.kind == FlexItemKind::Container && item.container) {
            ScaleContainerBounds(*item.container, inv);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        }
    }
}

void ScaleContainerBounds(Container& container, float inv) {
    InputBounds cb = container.GetBounds();
    ScaleBounds(cb, inv);
    container.SetBounds(cb);
    container.SetPixelScale(container.GetPixelScale() * inv);
    ScaleScrollbar(container.GetScrollbar(), inv);
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        ScaleFlexboxBounds(container.GetFlexbox(), inv);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        ScaleGridBounds(container.GetGrid(), inv);
    } else {
        ScalePositionBounds(container.GetContent(), inv);
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
        } else if (item.kind == GridItemKind::Container && item.container) {
            ScaleContainerBounds(*item.container, inv);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            ScaleGridBounds(*item.grid, inv);
        }
    }
}

template <typename Visit>
bool VisitScrollbars(Flexbox& flexbox, Visit& visit);
template <typename Visit>
bool VisitScrollbars(Grid& grid, Visit& visit);
template <typename Visit>
bool VisitScrollbars(Position& position, Visit& visit);
template <typename Visit>
bool VisitScrollbars(Container& container, Visit& visit);

// Walks every scrollbar in a nested container tree, including any living inside
// Flexbox / Grid / Position children.
template <typename Visit>
bool VisitScrollbars(Flexbox& flexbox, Visit& visit) {
    bool visited = false;
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Scrollbar) {
            visit(item.scrollbar);
            visited = true;
        } else if (item.kind == FlexItemKind::Container && item.container) {
            visited = VisitScrollbars(*item.container, visit) || visited;
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            visited = VisitScrollbars(*item.flexbox, visit) || visited;
        }
    }
    return visited;
}

template <typename Visit>
bool VisitScrollbars(Grid& grid, Visit& visit) {
    bool visited = false;
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Scrollbar) {
            visit(item.scrollbar);
            visited = true;
        } else if (item.kind == GridItemKind::Container && item.container) {
            visited = VisitScrollbars(*item.container, visit) || visited;
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            visited = VisitScrollbars(*item.flexbox, visit) || visited;
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            visited = VisitScrollbars(*item.grid, visit) || visited;
        }
    }
    return visited;
}

template <typename Visit>
bool VisitScrollbars(Position& position, Visit& visit) {
    bool visited = false;
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Scrollbar) {
            visit(item.scrollbar);
            visited = true;
        } else if (item.kind == PositionItemKind::Container && item.container) {
            visited = VisitScrollbars(*item.container, visit) || visited;
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            visited = VisitScrollbars(*item.flexbox, visit) || visited;
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            visited = VisitScrollbars(*item.grid, visit) || visited;
        } else if (item.kind == PositionItemKind::Position && item.position) {
            visited = VisitScrollbars(*item.position, visit) || visited;
        }
    }
    return visited;
}

// Visits every scrollbar in the scene tree, at any nesting depth.
template <typename Visit>
void VisitSceneScrollbars(std::vector<SceneItem>& items, Visit& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::Scrollbar) {
            visit(item.scrollbar);
        } else if (item.kind == SceneItemKind::Container) {
            VisitScrollbars(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox) {
            VisitScrollbars(item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid) {
            VisitScrollbars(item.grid, visit);
        } else if (item.kind == SceneItemKind::Position) {
            VisitScrollbars(item.position, visit);
        }
    }
}

// A container owns a scrollbar, and its clipped content may hold more of them.
template <typename Visit>
bool VisitScrollbars(Container& container, Visit& visit) {
    visit(container.GetScrollbar());
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        VisitScrollbars(container.GetFlexbox(), visit);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        VisitScrollbars(container.GetGrid(), visit);
    } else {
        VisitScrollbars(container.GetContent(), visit);
    }
    return true;
}

// Containers can be nested at any depth, so the finders below are mutually
// recursive and need forward declarations.
Button* FindButtonInContainer(Container& container, float x, float y);
Input* FindInputInContainer(Container& container, float x, float y);
Scrollbar* FindScrollbarInContainer(Container& container, float x, float y);
Container* FindContainerInContainer(Container& container, float x, float y);
const Button* FindButtonInContainerConst(const Container& container, float x, float y);
const Input* FindInputInContainerConst(const Container& container, float x, float y);
const Scrollbar* FindScrollbarInContainerConst(const Container& container, float x, float y);
Container* FindContainerInFlexbox(Flexbox& flexbox, float x, float y);
Container* FindContainerInGrid(Grid& grid, float x, float y);
Container* FindContainerInPosition(Position& position, float x, float y);
void CollectContainersIn(Container& container, float x, float y, std::vector<Container*>& out);
void CollectContainersIn(Flexbox& flexbox, float x, float y, std::vector<Container*>& out);
void CollectContainersIn(Grid& grid, float x, float y, std::vector<Container*>& out);
void CollectContainersIn(Position& position, float x, float y, std::vector<Container*>& out);

Button* FindButtonInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == FlexItemKind::Container && item.container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
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
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == FlexItemKind::Container && item.container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == FlexItemKind::Container && item.container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == GridItemKind::Container && item.container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == GridItemKind::Container && item.container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
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
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == GridItemKind::Container && item.container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == GridItemKind::Container && item.container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == PositionItemKind::Container && item.container) {
            ScaleContainerBounds(*item.container, inv);
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
        } else if (item.kind == PositionItemKind::Container && item.container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == PositionItemKind::Container && item.container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
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
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == PositionItemKind::Container && item.container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == PositionItemKind::Container && item.container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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

// --- Container lookups -------------------------------------------------------
// A container clips its content, so a point outside its body can neither hit a
// child control nor start a scroll there.

Button* FindButtonInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindButtonInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindButtonInGrid(container.GetGrid(), x, y);
    }
    return FindButtonInPosition(container.GetContent(), x, y);
}

Input* FindInputInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindInputInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindInputInGrid(container.GetGrid(), x, y);
    }
    return FindInputInPosition(container.GetContent(), x, y);
}

Scrollbar* FindScrollbarInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    Scrollbar& bar = container.GetScrollbar();
    if (container.GetStyle().showScrollbar && bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
        return &bar;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindScrollbarInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindScrollbarInGrid(container.GetGrid(), x, y);
    }
    return FindScrollbarInPosition(container.GetContent(), x, y);
}

const Button* FindButtonInContainerConst(const Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindButtonInFlexboxConst(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindButtonInGridConst(container.GetGrid(), x, y);
    }
    return FindButtonInPositionConst(container.GetContent(), x, y);
}

const Input* FindInputInContainerConst(const Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindInputInFlexboxConst(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindInputInGridConst(container.GetGrid(), x, y);
    }
    return FindInputInPositionConst(container.GetContent(), x, y);
}

const Scrollbar* FindScrollbarInContainerConst(const Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    const Scrollbar& bar = container.GetScrollbar();
    if (container.GetStyle().showScrollbar && bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
        return &bar;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindScrollbarInFlexboxConst(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindScrollbarInGridConst(container.GetGrid(), x, y);
    }
    return FindScrollbarInPositionConst(container.GetContent(), x, y);
}

Container* FindContainerInFlexbox(Flexbox& flexbox, float x, float y) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Container && item.container) {
            Container* found = FindContainerInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Container* found = FindContainerInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Container* FindContainerInGrid(Grid& grid, float x, float y) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Container && item.container) {
            Container* found = FindContainerInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Container* found = FindContainerInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Container* found = FindContainerInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Container* FindContainerInPosition(Position& position, float x, float y) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Container && item.container) {
            Container* found = FindContainerInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Container* found = FindContainerInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Container* found = FindContainerInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Container* found = FindContainerInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

// The deepest container under the point, which is the one a wheel event scrolls.
Container* FindContainerInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    Container* nested = nullptr;
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        nested = FindContainerInFlexbox(container.GetFlexbox(), x, y);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        nested = FindContainerInGrid(container.GetGrid(), x, y);
    } else {
        nested = FindContainerInPosition(container.GetContent(), x, y);
    }
    return nested ? nested : &container;
}

// Containers under a point, innermost first, so wheel scrolling can bubble out
// of a region that has nothing left to scroll.
void CollectContainersIn(Container& container, float x, float y, std::vector<Container*>& out) {
    if (!container.GetBounds().Contains(x, y)) {
        return;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        CollectContainersIn(container.GetFlexbox(), x, y, out);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        CollectContainersIn(container.GetGrid(), x, y, out);
    } else {
        CollectContainersIn(container.GetContent(), x, y, out);
    }
    out.push_back(&container);
}

void CollectContainersIn(Flexbox& flexbox, float x, float y, std::vector<Container*>& out) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Container && item.container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        }
    }
}

void CollectContainersIn(Grid& grid, float x, float y, std::vector<Container*>& out) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Container && item.container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            CollectContainersIn(*item.grid, x, y, out);
        }
    }
}

void CollectContainersIn(Position& position, float x, float y, std::vector<Container*>& out) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Container && item.container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            CollectContainersIn(*item.grid, x, y, out);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            CollectContainersIn(*item.position, x, y, out);
        }
    }
}

void CollectSceneContainers(std::vector<SceneItem>& items, float x, float y, std::vector<Container*>& out) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::Container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == SceneItemKind::Flexbox) {
            CollectContainersIn(item.flexbox, x, y, out);
        } else if (item.kind == SceneItemKind::Grid) {
            CollectContainersIn(item.grid, x, y, out);
        } else if (item.kind == SceneItemKind::Position) {
            CollectContainersIn(item.position, x, y, out);
        }
    }
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
    focusedScrollbar_ = nullptr;
    panningContainer_ = nullptr;
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

void RoboScene::AddContainer(Container container) {
    SceneItem item;
    item.kind = SceneItemKind::Container;
    item.container = std::make_shared<Container>(std::move(container));
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

void RoboScene::AddSvg(Svg svg) {
    SceneItem item;
    item.kind = SceneItemKind::Svg;
    item.svg = std::move(svg);
    items_.push_back(std::move(item));
}

bool RoboScene::RerenderIfPossible() {
    if (!hasRenderContext_ || !platform_ || !fonts_) {
        return false;
    }
    MarkFullDirty();
    return true;
}

void RoboScene::MarkFullDirty() {
    needsRender_ = true;
    fullDirty_ = true;
    damageOpen_ = false;
}

void RoboScene::MarkDirty(float x, float y, float width, float height) {
    needsRender_ = true;
    if (fullDirty_ || !frameReady_ || width <= 0.0f || height <= 0.0f) {
        fullDirty_ = true;
        damageOpen_ = false;
        return;
    }
    const float pad = 32.0f;
    const float left = x - pad;
    const float top = y - pad;
    const float right = x + width + pad;
    const float bottom = y + height + pad;
    if (!damageOpen_) {
        dirtyLeft_ = left;
        dirtyTop_ = top;
        dirtyRight_ = right;
        dirtyBottom_ = bottom;
        damageOpen_ = true;
    } else {
        dirtyLeft_ = std::min(dirtyLeft_, left);
        dirtyTop_ = std::min(dirtyTop_, top);
        dirtyRight_ = std::max(dirtyRight_, right);
        dirtyBottom_ = std::max(dirtyBottom_, bottom);
    }
    const float area = std::max(0.0f, dirtyRight_ - dirtyLeft_) * std::max(0.0f, dirtyBottom_ - dirtyTop_);
    const float surface = static_cast<float>(std::max(1, paintedWidth_)) * static_cast<float>(std::max(1, paintedHeight_));
    if (area > surface * 0.5f) {
        fullDirty_ = true;
        damageOpen_ = false;
    }
}

void RoboScene::DirtyButton(const Button* button) {
    if (!button) {
        return;
    }
    const ButtonBounds& bounds = button->GetBounds();
    MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
}

void RoboScene::DirtyInput(const Input* input) {
    if (!input) {
        return;
    }
    const InputBounds& bounds = input->GetBounds();
    MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
}

void RoboScene::DirtyScrollbar(const Scrollbar* scrollbar) {
    if (!scrollbar) {
        return;
    }
    const ScrollbarBounds& bounds = scrollbar->GetBounds();
    MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
}

void RoboScene::DirtySelectionVisual() {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (TextSelection::VisualBounds(x, y, width, height)) {
        MarkDirty(x, y, width, height);
    }
}

bool RoboScene::CanPaintPartial(float paintScale, int width, int height) const {
    if (fullDirty_ || !frameReady_ || !damageOpen_) {
        return false;
    }
    if (width != paintedWidth_ || height != paintedHeight_) {
        return false;
    }
    if (std::fabs(paintScale - paintedScale_) > 0.001f) {
        return false;
    }
    if (std::fabs(scrollX_ - paintedScrollX_) > 0.5f || std::fabs(scrollY_ - paintedScrollY_) > 0.5f) {
        return false;
    }
    return dirtyRight_ > dirtyLeft_ && dirtyBottom_ > dirtyTop_;
}

bool RoboScene::NeedsRender() const {
    return needsRender_;
}

void RoboScene::ConfigurePageBars() {
    if (pageBarsReady_) {
        return;
    }
    auto makeStyle = [](ScrollbarOrientation orientation) {
        ScrollbarStyle style;
        style.orientation = orientation;
        style.thickness = 14.0;
        style.cornerRadius = 7.0;
        style.minThumbLength = 28.0;
        style.showArrows = true;
        style.arrowSize = 16.0;
        style.autoHide = false;
        style.visibility = ScrollbarVisibility::Auto;
        return style;
    };
    pageVertical_.SetStyle(makeStyle(ScrollbarOrientation::Vertical));
    pageVertical_.ApplyVariantDefaults();
    pageHorizontal_.SetStyle(makeStyle(ScrollbarOrientation::Horizontal));
    pageHorizontal_.ApplyVariantDefaults();
    pageBarsReady_ = true;
}

void RoboScene::PullPageScroll() {
    scrollY_ = maxScrollY_ > 0.0f ? static_cast<float>(pageVertical_.GetValue()) * maxScrollY_ : 0.0f;
    scrollX_ = maxScrollX_ > 0.0f ? static_cast<float>(pageHorizontal_.GetValue()) * maxScrollX_ : 0.0f;
}

bool RoboScene::PageBarDriving(const Scrollbar& bar) const {
    return bar.IsDragging() || bar.NeedsAnimation() || bar.GetPressedArrow() != ScrollbarArrow::None;
}

void RoboScene::SetFoundation(const std::string& name) {
    foundation_ = name;
    MarkFullDirty();
}

void RoboScene::SetFoundationForTheme(Theme theme, const std::string& name) {
    if (theme == Theme::Dark) {
        darkFoundation_ = name;
    } else {
        lightFoundation_ = name;
    }
    MarkFullDirty();
}

void RoboScene::SetTheme(Theme theme) {
    if (theme_ == theme) {
        return;
    }
    theme_ = theme;
    MarkFullDirty();
}

const Foundation& RoboScene::ActiveFoundation() const {
    const std::string* name = &foundation_;
    if (theme_ == Theme::Dark && !darkFoundation_.empty()) {
        name = &darkFoundation_;
    } else if (theme_ == Theme::Light && !lightFoundation_.empty()) {
        name = &lightFoundation_;
    }
    const Foundation* found = Foundation::Find(*name);
    if (found == nullptr) {
        return Foundation::RoboFoundation();
    }
    return *found;
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) {
    const FoundationScope foundationScope(ActiveFoundation(), theme_);
    platform_ = &platform;
    fonts_ = &fonts;
    background_ = background;
    padding_ = padding;
    hasRenderContext_ = true;
    needsRender_ = false;
    ConfigurePageBars();

    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    surfaceWidth_ = width;
    surfaceHeight_ = height;
    // The surface is already device pixels and matches the screen. Page overflow
    // scrolls inside this canvas; the canvas itself does not grow.
    const float paintScale = std::max(0.5f, platform.GetPixelScale() * appZoom_);
    if (lastPaintScale_ > 0.0f) {
        const float ratio = paintScale / lastPaintScale_;
        if (std::fabs(ratio - 1.0f) > 0.001f) {
            scrollX_ *= ratio;
            scrollY_ *= ratio;
            maxScrollX_ *= ratio;
            maxScrollY_ *= ratio;
        }
    }
    lastPaintScale_ = paintScale;
    const float pad = padding * paintScale;
    const float barThickness = 14.0f * paintScale;
    const float edge = 3.0f * paintScale;

    if (frame_.GetWidth() != width || frame_.GetHeight() != height) {
        frame_.Resize(width, height);
    }
    FrameBuffer& frame = frame_;
    const bool partial = CanPaintPartial(paintScale, width, height);
    int damageX = 0;
    int damageY = 0;
    int damageWidth = width;
    int damageHeight = height;
    if (partial) {
        damageX = std::max(0, static_cast<int>(std::floor(dirtyLeft_)));
        damageY = std::max(0, static_cast<int>(std::floor(dirtyTop_)));
        const int damageRight = std::min(width, static_cast<int>(std::ceil(dirtyRight_)));
        const int damageBottom = std::min(height, static_cast<int>(std::ceil(dirtyBottom_)));
        damageWidth = std::max(0, damageRight - damageX);
        damageHeight = std::max(0, damageBottom - damageY);
        frame.PushClipRect(static_cast<float>(damageX), static_cast<float>(damageY),
                           static_cast<float>(damageWidth), static_cast<float>(damageHeight));
        frame.ClearRect(damageX, damageY, damageWidth, damageHeight, background);
    }

    float verticalReserve = partial ? paintedVerticalReserve_ : 0.0f;
    float horizontalReserve = partial ? paintedHorizontalReserve_ : 0.0f;
    for (int pass = 0; pass < 3; ++pass) {
        scrollX_ = std::max(0.0f, std::min(scrollX_, maxScrollX_));
        scrollY_ = std::max(0.0f, std::min(scrollY_, maxScrollY_));

        if (!partial) {
            frame.Clear(background);
        }
        TextSelection::BeginFrame();

        const float contentWidth = std::max(0.0f, static_cast<float>(width) - pad * 2.0f - verticalReserve);
        const float originX = pad - scrollX_;
        float cursorY = pad - scrollY_;
        float contentRight = originX + scrollX_;

        for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
            SceneItem& item = items_[static_cast<std::size_t>(i)];
            if (item.kind == SceneItemKind::Text) {
                cursorY += TextPainter::Paint(frame, fonts, item.text, originX, cursorY, contentWidth, paintScale);
                contentRight = std::max(contentRight, originX + contentWidth + scrollX_);
            } else if (item.kind == SceneItemKind::Button) {
                const ButtonPaintResult painted =
                    ButtonPainter::Paint(frame, fonts, item.button, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const ButtonBounds& bounds = item.button.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Input) {
                if (&item.input == focusedInput_) {
                    item.input.SetInteraction(InputInteraction::Focused);
                } else if (&item.input == hoveredInput_ && item.input.GetInteraction() != InputInteraction::Focused) {
                    item.input.SetInteraction(InputInteraction::Hovered);
                } else if (&item.input != focusedInput_) {
                    item.input.SetInteraction(InputInteraction::Normal);
                }

                const InputPaintResult painted =
                    InputPainter::Paint(frame, fonts, item.input, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.input.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Scrollbar) {
                if (&item.scrollbar == draggingScrollbar_) {
                    item.scrollbar.SetInteraction(ScrollbarInteraction::Dragging);
                } else if (&item.scrollbar == hoveredScrollbar_) {
                    item.scrollbar.SetInteraction(ScrollbarInteraction::Hovered);
                } else {
                    item.scrollbar.SetInteraction(ScrollbarInteraction::Normal);
                }

                const ScrollbarPaintResult painted =
                    ScrollbarPainter::Paint(frame, item.scrollbar, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const ScrollbarBounds& bounds = item.scrollbar.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Container) {
                const ContainerPaintResult painted =
                    ContainerPainter::Paint(frame, fonts, *item.container, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.container->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Flexbox) {
                const FlexboxPaintResult painted =
                    FlexboxPainter::Paint(frame, fonts, item.flexbox, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const FlexboxBounds& bounds = item.flexbox.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Grid) {
                const GridPaintResult painted =
                    GridPainter::Paint(frame, fonts, item.grid, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const GridBounds& bounds = item.grid.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Svg) {
                const SvgPaintResult painted =
                    SvgPainter::Paint(frame, item.svg, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const SvgBounds& bounds = item.svg.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Position) {
                const PositionPaintResult painted =
                    PositionPainter::Paint(frame, fonts, item.position, originX, cursorY, contentWidth, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.position.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            }
        }

        const float contentHeight = cursorY + scrollY_ + pad;
        const float viewportHeight = std::max(1.0f, static_cast<float>(height) - horizontalReserve);
        const float viewportWidth = std::max(1.0f, static_cast<float>(width) - verticalReserve);
        const float nextMaxY = std::max(0.0f, contentHeight - viewportHeight);
        const float nextMaxX = std::max(0.0f, contentRight + pad - viewportWidth);
        const float nextVertical = nextMaxY > 1.0f ? barThickness : 0.0f;
        const float nextHorizontal = nextMaxX > 1.0f ? barThickness : 0.0f;
        const bool scrollClamped = scrollY_ > nextMaxY + 0.5f || scrollX_ > nextMaxX + 0.5f;
        maxScrollY_ = nextMaxY;
        maxScrollX_ = nextMaxX;
        scrollY_ = std::max(0.0f, std::min(scrollY_, maxScrollY_));
        scrollX_ = std::max(0.0f, std::min(scrollX_, maxScrollX_));
        if (!partial && pass < 2 && (nextVertical != verticalReserve || nextHorizontal != horizontalReserve || scrollClamped)) {
            verticalReserve = nextVertical;
            horizontalReserve = nextHorizontal;
            continue;
        }
        verticalReserve = nextVertical;
        horizontalReserve = nextHorizontal;
        break;
    }

    const auto syncBar = [](Scrollbar& bar, float scroll, float maxScroll, float viewport) {
        bar.SetContentSize(static_cast<double>(maxScroll + viewport));
        bar.SetViewportSize(static_cast<double>(viewport));
        if (!bar.IsDragging() && !bar.NeedsAnimation() && bar.GetPressedArrow() == ScrollbarArrow::None) {
            bar.SetValue(maxScroll > 0.0f ? static_cast<double>(scroll / maxScroll) : 0.0);
        }
    };
    syncBar(pageVertical_, scrollY_, maxScrollY_, static_cast<float>(height));
    syncBar(pageHorizontal_, scrollX_, maxScrollX_, static_cast<float>(width));

    const auto paintPageBar = [&](Scrollbar& bar, float x, float y, float barWidth, float barHeight) {
        if (bar.IsDragging() || draggingScrollbar_ == &bar) {
            bar.SetInteraction(ScrollbarInteraction::Dragging);
        } else if (hoveredScrollbar_ == &bar) {
            bar.SetInteraction(ScrollbarInteraction::Hovered);
        } else {
            bar.SetInteraction(ScrollbarInteraction::Normal);
        }
        ScrollbarPainter::Paint(frame, bar, x, y, barWidth, paintScale, barWidth, barHeight);
    };

    if (maxScrollY_ > 1.0f) {
        paintPageBar(pageVertical_,
                     static_cast<float>(width) - barThickness,
                     edge,
                     barThickness,
                     std::max(0.0f, static_cast<float>(height) - edge * 2.0f - horizontalReserve));
    } else {
        pageVertical_.SetBounds({});
    }
    if (maxScrollX_ > 1.0f) {
        paintPageBar(pageHorizontal_,
                     edge,
                     static_cast<float>(height) - barThickness,
                     std::max(0.0f, static_cast<float>(width) - edge * 2.0f - verticalReserve),
                     barThickness);
    } else {
        pageHorizontal_.SetBounds({});
    }

    TextSelection::EndFrame();
    if (TextSelection::ShowsHandles()) {
        float startX = 0.0f;
        float startY = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        float lineHeight = 0.0f;
        if (TextSelection::HandleGeometry(startX, startY, endX, endY, lineHeight)) {
            const float radius = std::max(5.0f, lineHeight * 0.22f);
            const Color handle = Color::FromHex("#007AFF");
            frame.FillRoundedRect(startX - radius, startY + lineHeight - radius * 0.15f, radius * 2.0f, radius * 2.0f,
                                  radius, handle, 1.0f);
            frame.FillRoundedRect(endX - radius, endY + lineHeight - radius * 0.15f, radius * 2.0f, radius * 2.0f,
                                  radius, handle, 1.0f);
        }
    }
    if (partial) {
        frame.PopClip();
    }
    if (!partial || damageWidth <= 0 || damageHeight <= 0) {
        damageX = 0;
        damageY = 0;
        damageWidth = width;
        damageHeight = height;
    }
    frame.SetPresentRect(damageX, damageY, damageWidth, damageHeight);
    fullDirty_ = false;
    damageOpen_ = false;
    frameReady_ = true;
    paintedScrollX_ = scrollX_;
    paintedScrollY_ = scrollY_;
    paintedScale_ = paintScale;
    paintedWidth_ = width;
    paintedHeight_ = height;
    paintedVerticalReserve_ = verticalReserve;
    paintedHorizontalReserve_ = horizontalReserve;
    platform.Present(frame);
}

bool RoboScene::ScrollBy(float deltaY) {
    if (!hasRenderContext_ || deltaY == 0.0f || maxScrollY_ <= 0.0f) {
        return false;
    }
    if (!pageVertical_.ScrollByPixels(static_cast<double>(deltaY))) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::ZoomTo(float zoom, float anchorX, float anchorY) {
    constexpr float MIN_ZOOM = 0.5f;
    constexpr float MAX_ZOOM = 3.0f;
    if (!std::isfinite(zoom) || !std::isfinite(anchorX) || !std::isfinite(anchorY)) {
        return false;
    }
    const float next = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom));
    if (std::fabs(next - appZoom_) < 0.0005f) {
        return false;
    }
    const float ratio = next / std::max(MIN_ZOOM, appZoom_);
    scrollX_ = (scrollX_ + anchorX) * ratio - anchorX;
    scrollY_ = (scrollY_ + anchorY) * ratio - anchorY;
    if (scrollX_ < 0.0f) {
        scrollX_ = 0.0f;
    }
    if (scrollY_ < 0.0f) {
        scrollY_ = 0.0f;
    }
    maxScrollX_ *= ratio;
    maxScrollY_ *= ratio;
    appZoom_ = next;
    RerenderIfPossible();
    return true;
}

bool RoboScene::ZoomBy(float factor, float anchorX, float anchorY) {
    if (!(factor > 0.0f)) {
        return false;
    }
    return ZoomTo(appZoom_ * factor, anchorX, anchorY);
}

float RoboScene::GetZoom() const {
    return appZoom_;
}

bool RoboScene::ScrollByX(float deltaX) {
    if (!hasRenderContext_ || deltaX == 0.0f || maxScrollX_ <= 0.0f) {
        return false;
    }
    if (!pageHorizontal_.ScrollByPixels(static_cast<double>(deltaX))) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

Button* RoboScene::FindButtonAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == SceneItemKind::Container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == SceneItemKind::Container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
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

Scrollbar* RoboScene::PageScrollbarAt(float x, float y) {
    if (pageVertical_.IsVisible() && pageVertical_.GetBounds().Contains(x, y)) {
        return &pageVertical_;
    }
    if (pageHorizontal_.IsVisible() && pageHorizontal_.GetBounds().Contains(x, y)) {
        return &pageHorizontal_;
    }
    return nullptr;
}

const Scrollbar* RoboScene::PageScrollbarAt(float x, float y) const {
    if (pageVertical_.IsVisible() && pageVertical_.GetBounds().Contains(x, y)) {
        return &pageVertical_;
    }
    if (pageHorizontal_.IsVisible() && pageHorizontal_.GetBounds().Contains(x, y)) {
        return &pageHorizontal_;
    }
    return nullptr;
}

Scrollbar* RoboScene::FindScrollbarAt(float x, float y) {
    if (Scrollbar* pageBar = PageScrollbarAt(x, y)) {
        return pageBar;
    }
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == SceneItemKind::Container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == SceneItemKind::Container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
        } else if (item.kind == SceneItemKind::Container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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
    if (const Scrollbar* pageBar = PageScrollbarAt(x, y)) {
        return pageBar;
    }
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == SceneItemKind::Container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
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

bool RoboScene::SetFocusedScrollbar(Scrollbar* scrollbar) {
    if (focusedScrollbar_ == scrollbar) {
        return false;
    }
    DirtyScrollbar(focusedScrollbar_);
    if (focusedScrollbar_) {
        focusedScrollbar_->SetFocused(false);
        if (focusedScrollbar_ != hoveredScrollbar_ && focusedScrollbar_ != draggingScrollbar_) {
            focusedScrollbar_->SetInteraction(ScrollbarInteraction::Normal);
        }
    }
    focusedScrollbar_ = scrollbar;
    if (focusedScrollbar_) {
        focusedScrollbar_->SetFocused(true);
        if (focusedScrollbar_ != draggingScrollbar_) {
            focusedScrollbar_->SetInteraction(ScrollbarInteraction::Hovered);
        }
    }
    DirtyScrollbar(focusedScrollbar_);
    return true;
}

Scrollbar* RoboScene::ActiveScrollbar() {
    // A control the user explicitly interacted with wins over a passing hover,
    // so keyboard scrolling keeps going to the last one that was used.
    if (focusedScrollbar_) {
        return focusedScrollbar_;
    }
    return hoveredScrollbar_;
}

Container* RoboScene::FindContainerAt(float x, float y) {
    // Scene items are painted in order, so later ones sit on top.
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        Container* found = nullptr;
        if (item.kind == SceneItemKind::Container) {
            found = FindContainerInContainer(*item.container, x, y);
        } else if (item.kind == SceneItemKind::Flexbox) {
            found = FindContainerInFlexbox(item.flexbox, x, y);
        } else if (item.kind == SceneItemKind::Grid) {
            found = FindContainerInGrid(item.grid, x, y);
        } else if (item.kind == SceneItemKind::Position) {
            found = FindContainerInPosition(item.position, x, y);
        }
        if (found) {
            return found;
        }
    }
    return nullptr;
}

Scrollbar* RoboScene::ScrollTargetAt(float x, float y) {
    // Wheel scrolling goes to the innermost scroll region under the cursor and
    // bubbles outwards while that region has nothing left to scroll.
    std::vector<Container*> containers;
    CollectSceneContainers(items_, x, y, containers);
    for (Container* container : containers) {
        if (container->GetStyle().showScrollbar && container->IsOverflowing()) {
            return &container->GetScrollbar();
        }
    }
    if (Scrollbar* bar = FindScrollbarAt(x, y)) {
        return bar;
    }
    return FindScrollbarNear(x, y, 28.0f);
}

Scrollbar* RoboScene::FindScrollbarNear(float x, float y, float tolerance) {
    Scrollbar* best = nullptr;
    auto visit = [&](Scrollbar& scrollbar) {
        if (!scrollbar.IsVisible()) {
            return;
        }
        ScrollbarBounds bounds = scrollbar.GetBounds();
        if (scrollbar.GetOrientation() == ScrollbarOrientation::Vertical) {
            bounds.x -= tolerance;
            bounds.width += tolerance * 2.0f;
        } else {
            bounds.y -= tolerance;
            bounds.height += tolerance * 2.0f;
        }
        if (bounds.Contains(x, y)) {
            best = &scrollbar;  // Later controls are drawn on top, so they win.
        }
    };
    VisitSceneScrollbars(items_, visit);
    visit(pageVertical_);
    visit(pageHorizontal_);
    return best;
}

bool RoboScene::Wheel(float x, float y, float deltaX, float deltaY) {
    // Prefer the bar whose axis matches the gesture, then any scroller under
    // the cursor. A mouse wheel over a horizontal bar still moves it.
    const bool wantHorizontal = std::fabs(deltaX) > std::fabs(deltaY);
    Scrollbar* target = nullptr;
    std::vector<Container*> containers;
    CollectSceneContainers(items_, x, y, containers);
    for (Container* container : containers) {
        if (!container->GetStyle().showScrollbar || !container->IsOverflowing()) {
            continue;
        }
        const bool horizontal = container->GetScrollbar().GetOrientation() == ScrollbarOrientation::Horizontal;
        if (horizontal == wantHorizontal) {
            target = &container->GetScrollbar();
            break;
        }
    }
    if (!target) {
        target = ScrollTargetAt(x, y);
    }
    if (!target) {
        bool moved = false;
        if (maxScrollY_ > 0.0f && deltaY != 0.0f) {
            moved = pageVertical_.ScrollByPixels(static_cast<double>(deltaY)) || moved;
        }
        if (maxScrollX_ > 0.0f && deltaX != 0.0f) {
            moved = pageHorizontal_.ScrollByPixels(static_cast<double>(deltaX)) || moved;
        }
        if (moved) {
            RerenderIfPossible();
        }
        return moved;
    }

    SetHoveredScrollbar(target);
    SetFocusedScrollbar(target);
    const bool horizontal = target->GetOrientation() == ScrollbarOrientation::Horizontal;
    double pixels = 0.0;
    if (horizontal) {
        pixels = std::fabs(deltaX) >= std::fabs(deltaY) ? static_cast<double>(deltaX) : static_cast<double>(deltaY);
    } else {
        pixels = std::fabs(deltaY) >= std::fabs(deltaX) ? static_cast<double>(deltaY) : static_cast<double>(deltaX);
    }
    if (pixels == 0.0) {
        return false;
    }
    const bool moved = target->ScrollByPixels(pixels);
    target->SetHotArrow(ScrollbarArrow::None);
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

int RoboScene::ActiveScrollbarAxis() const {
    const Scrollbar* target = focusedScrollbar_ ? focusedScrollbar_ : hoveredScrollbar_;
    if (!target || !target->IsVisible()) {
        return 0;
    }
    return target->GetOrientation() == ScrollbarOrientation::Vertical ? 1 : 2;
}

bool RoboScene::OwnsScrollGesture(float x, float y) {
    if (FindScrollbarAt(x, y)) {
        return true;
    }
    std::vector<Container*> containers;
    CollectSceneContainers(items_, x, y, containers);
    for (Container* container : containers) {
        if (container->GetStyle().showScrollbar && container->IsOverflowing()) {
            return true;
        }
    }
    return false;
}

bool RoboScene::KeyScrollLine(int direction) {
    Scrollbar* target = ActiveScrollbar();
    if (!target || direction == 0) {
        return false;
    }
    const bool moved = target->ScrollByLines(static_cast<double>(direction));
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

bool RoboScene::KeyScrollPage(int direction) {
    Scrollbar* target = ActiveScrollbar();
    if (!target || direction == 0) {
        return false;
    }
    const bool moved = target->ScrollByPage(static_cast<double>(direction));
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

bool RoboScene::KeyScrollToStart() {
    Scrollbar* target = ActiveScrollbar();
    if (!target) {
        return false;
    }
    const bool moved = target->ScrollToStart();
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

bool RoboScene::KeyScrollToEnd() {
    Scrollbar* target = ActiveScrollbar();
    if (!target) {
        return false;
    }
    const bool moved = target->ScrollToEnd();
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
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
    DirtyButton(hoveredButton_);
    if (hoveredButton_ && hoveredButton_ != pressedButton_) {
        hoveredButton_->SetInteraction(ButtonInteraction::Normal);
    }
    hoveredButton_ = button;
    if (hoveredButton_ && hoveredButton_ != pressedButton_) {
        hoveredButton_->SetInteraction(ButtonInteraction::Hovered);
    }
    DirtyButton(hoveredButton_);
    return true;
}

bool RoboScene::SetHoveredInput(Input* input) {
    if (hoveredInput_ == input) {
        return false;
    }
    DirtyInput(hoveredInput_);
    if (hoveredInput_ && hoveredInput_ != focusedInput_) {
        hoveredInput_->SetInteraction(InputInteraction::Normal);
    }
    hoveredInput_ = input;
    if (hoveredInput_ && hoveredInput_ != focusedInput_) {
        hoveredInput_->SetInteraction(InputInteraction::Hovered);
    }
    DirtyInput(hoveredInput_);
    return true;
}

bool RoboScene::SetHoveredScrollbar(Scrollbar* scrollbar) {
    if (hoveredScrollbar_ == scrollbar) {
        if (scrollbar) {
            // Pointer stayed on the same control: keep it revealed.
            scrollbar->NotifyInteraction();
        }
        return false;
    }
    DirtyScrollbar(hoveredScrollbar_);
    if (hoveredScrollbar_) {
        hoveredScrollbar_->SetPointerInside(false);
        hoveredScrollbar_->SetHotArrow(ScrollbarArrow::None);
        if (hoveredScrollbar_ != draggingScrollbar_) {
            hoveredScrollbar_->SetInteraction(hoveredScrollbar_->IsFocused() ? ScrollbarInteraction::Hovered
                                                                             : ScrollbarInteraction::Normal);
        }
    }
    hoveredScrollbar_ = scrollbar;
    if (hoveredScrollbar_) {
        hoveredScrollbar_->SetPointerInside(true);
        if (hoveredScrollbar_ != draggingScrollbar_) {
            hoveredScrollbar_->SetInteraction(ScrollbarInteraction::Hovered);
        }
    }
    DirtyScrollbar(hoveredScrollbar_);
    return true;
}

bool RoboScene::SetPressedButton(Button* button) {
    if (pressedButton_ == button) {
        return false;
    }
    DirtyButton(pressedButton_);
    if (pressedButton_) {
        const ButtonInteraction next =
            (pressedButton_ == hoveredButton_) ? ButtonInteraction::Hovered : ButtonInteraction::Normal;
        pressedButton_->SetInteraction(next);
    }
    pressedButton_ = button;
    if (pressedButton_) {
        pressedButton_->SetInteraction(ButtonInteraction::Pressed);
    }
    DirtyButton(pressedButton_);
    return true;
}

bool RoboScene::SetFocusedInput(Input* input) {
    if (focusedInput_ == input) {
        return false;
    }
    DirtyInput(focusedInput_);
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
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::PointerMove(float x, float y) {
    selectX_ = x;
    selectY_ = y;
    if (TextSelection::IsDragging()) {
        DirtySelectionVisual();
        TextSelection::MovePointer(x, y);
        DirtySelectionVisual();
        if (!needsRender_) {
            MarkFullDirty();
        }
        return true;
    }

    if (draggingScrollbar_) {
        draggingScrollbar_->UpdateDrag(x, y);
        if (draggingScrollbar_ == &pageVertical_ || draggingScrollbar_ == &pageHorizontal_) {
            PullPageScroll();
        }
        RerenderIfPossible();
        return true;
    }

    if (panningContainer_) {
        Scrollbar& bar = panningContainer_->GetScrollbar();
        const float dx = x - panX_;
        const float dy = y - panY_;
        panX_ = x;
        panY_ = y;
        const bool horizontal = bar.GetOrientation() == ScrollbarOrientation::Horizontal;
        const double delta = horizontal ? static_cast<double>(-dx) : static_cast<double>(-dy);
        if (bar.ScrollByPixelsImmediate(delta)) {
            RerenderIfPossible();
        }
        return true;
    }

    if (pressedButton_) {
        const bool inside = FindButtonAt(x, y) == pressedButton_;
        const ButtonInteraction next = inside ? ButtonInteraction::Pressed : ButtonInteraction::Hovered;
        if (pressedButton_->GetInteraction() != next) {
            pressedButton_->SetInteraction(next);
            DirtyButton(pressedButton_);
            return true;
        }
        return false;
    }

    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    Button* button = scrollbar ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || button) ? nullptr : FindInputAt(x, y);

    // Hovering anywhere over a scroll region counts as hovering the region's
    // bar, so auto-hide reveals it and the wheel knows which region to scroll.
    // An interactive child under the pointer keeps the hover it would get.
    if (!scrollbar && !button && !input) {
        if (Container* container = FindContainerAt(x, y)) {
            if (container->GetStyle().showScrollbar && container->GetScrollbar().IsVisible()) {
                scrollbar = &container->GetScrollbar();
            }
        }
    }

    bool changed = SetHoveredScrollbar(scrollbar);
    changed = SetHoveredButton(button) || changed;
    changed = SetHoveredInput(input) || changed;
    if (scrollbar) {
        // Track the arrowhead under the pointer so it can show its hover state.
        const ScrollbarArrow arrow = scrollbar->FindArrowAt(x, y);
        if (scrollbar->GetHotArrow() != arrow) {
            scrollbar->SetHotArrow(arrow);
            DirtyScrollbar(scrollbar);
            changed = true;
        }
    }
    return changed;
}

bool RoboScene::LongPressSelect(float x, float y) {
    selectX_ = x;
    selectY_ = y;
    float oldX = 0.0f;
    float oldY = 0.0f;
    float oldWidth = 0.0f;
    float oldHeight = 0.0f;
    const bool hadVisual = TextSelection::VisualBounds(oldX, oldY, oldWidth, oldHeight);
    TextSelection::LongPress(x, y);
    if (!TextSelection::IsDragging() && !TextSelection::HasSelection()) {
        return false;
    }
    if (hadVisual) {
        MarkDirty(oldX, oldY, oldWidth, oldHeight);
    }
    SetPressedButton(nullptr);
    SetFocusedInput(nullptr);
    SetFocusedScrollbar(nullptr);
    DirtySelectionVisual();
    return true;
}

bool RoboScene::PointerDown(float x, float y, bool fromTouch, int clicks) {
    panningContainer_ = nullptr;
    selectX_ = x;
    selectY_ = y;
    if (!fromTouch && TextSelection::BeginHandleDrag(x, y)) {
        DirtySelectionVisual();
        SetPressedButton(nullptr);
        SetFocusedInput(nullptr);
        SetFocusedScrollbar(nullptr);
        DirtySelectionVisual();
        return true;
    }
    if (!fromTouch && TextSelection::Hit(x, y) >= 0) {
        DirtySelectionVisual();
        TextSelection::BeginClick(x, y, clicks > 0 ? clicks : 1);
        SetPressedButton(nullptr);
        SetFocusedInput(nullptr);
        SetFocusedScrollbar(nullptr);
        DirtySelectionVisual();
        return true;
    }
    bool changed = false;
    if (TextSelection::HasCaret() || TextSelection::HasSelection()) {
        DirtySelectionVisual();
        TextSelection::Clear();
        changed = true;
    }
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    Button* button = scrollbar ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || button) ? nullptr : FindInputAt(x, y);
    // Clicking inside a container (not on its track) focuses its scrollbar so
    // keyboard and wheel scrolling land on that region without jumping it.
    Container* container = (scrollbar || button || input) ? nullptr : FindContainerAt(x, y);

    if (scrollbar) {
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedScrollbar(scrollbar) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        const ScrollbarArrow arrow = scrollbar->FindArrowAt(x, y);
        if (arrow != ScrollbarArrow::None) {
            // Clicking a filled arrowhead steps the value instead of dragging.
            scrollbar->SetPressedArrow(arrow);
            scrollbar->SetHotArrow(arrow);
            scrollbar->StepByArrow(arrow);
        } else {
            scrollbar->BeginDrag(x, y);
            draggingScrollbar_ = scrollbar;
        }
        changed = SetHoveredScrollbar(scrollbar) || changed;
        if (scrollbar == &pageVertical_ || scrollbar == &pageHorizontal_) {
            PullPageScroll();
        }
        MarkFullDirty();
        changed = true;
    } else if (button) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedScrollbar(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(button) || changed;
        changed = SetHoveredButton(button) || changed;
        changed = SetHoveredInput(nullptr) || changed;
    } else if (input) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedScrollbar(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedInput(input) || changed;
        changed = SetHoveredInput(input) || changed;
        changed = SetHoveredButton(nullptr) || changed;
    } else {
        Scrollbar* containerBar = nullptr;
        if (container && container->GetStyle().showScrollbar && container->GetScrollbar().IsVisible()) {
            containerBar = &container->GetScrollbar();
        }
        changed = SetFocusedScrollbar(containerBar) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        changed = SetHoveredScrollbar(containerBar) || changed;
        if (fromTouch && container && container->IsOverflowing()) {
            panningContainer_ = container;
            panX_ = x;
            panY_ = y;
            MarkFullDirty();
            changed = true;
        }
    }

    if (changed && !needsRender_) {
        MarkFullDirty();
    }
    return changed;
}

bool RoboScene::PointerUp(float x, float y) {
    bool changed = false;
    if (TextSelection::IsDragging()) {
        DirtySelectionVisual();
        TextSelection::EndDrag();
        changed = true;
    }
    if (draggingScrollbar_) {
        DirtyScrollbar(draggingScrollbar_);
        draggingScrollbar_->EndDrag();
        hoveredScrollbar_ = nullptr;
        draggingScrollbar_ = nullptr;
        changed = true;
    }
    if (panningContainer_) {
        panningContainer_ = nullptr;
        changed = true;
    }

    auto releaseArrows = [this, &changed](Scrollbar& scrollbar) {
        if (scrollbar.GetPressedArrow() != ScrollbarArrow::None) {
            scrollbar.SetPressedArrow(ScrollbarArrow::None);
            DirtyScrollbar(&scrollbar);
            changed = true;
        }
    };
    VisitSceneScrollbars(items_, releaseArrows);
    releaseArrows(pageVertical_);
    releaseArrows(pageHorizontal_);

    changed = SetPressedButton(nullptr) || changed;
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    Button* button = scrollbar ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || button) ? nullptr : FindInputAt(x, y);

    changed = SetHoveredScrollbar(scrollbar) || changed;
    changed = SetHoveredButton(button) || changed;
    changed = SetHoveredInput(input) || changed;
    if (changed && !needsRender_) {
        MarkFullDirty();
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
    if (panningContainer_) {
        panningContainer_ = nullptr;
        MarkFullDirty();
        changed = true;
    }
    if (changed && !needsRender_) {
        MarkFullDirty();
    }
    return changed;
}

bool RoboScene::Tick(double deltaMs) {
    bool changed = false;
    bool contentMoved = false;
    auto visit = [&changed, &contentMoved, this, deltaMs](Scrollbar& scrollbar) {
        const double value = scrollbar.GetValue();
        if (!scrollbar.Tick(deltaMs)) {
            return;
        }
        changed = true;
        if (std::fabs(scrollbar.GetValue() - value) > 0.00001) {
            contentMoved = true;
        } else {
            DirtyScrollbar(&scrollbar);
        }
    };
    VisitSceneScrollbars(items_, visit);
    if (TextSelection::Tick(deltaMs)) {
        DirtySelectionVisual();
        changed = true;
    }
    if (TextSelection::IsDragging()) {
        const float margin = 24.0f;
        float dx = 0.0f;
        float dy = 0.0f;
        if (selectY_ < margin) {
            dy = -std::min(28.0f, margin - selectY_);
        } else if (selectY_ > static_cast<float>(surfaceHeight_) - margin) {
            dy = std::min(28.0f, selectY_ - (static_cast<float>(surfaceHeight_) - margin));
        }
        if (selectX_ < margin) {
            dx = -std::min(28.0f, margin - selectX_);
        } else if (selectX_ > static_cast<float>(surfaceWidth_) - margin) {
            dx = std::min(28.0f, selectX_ - (static_cast<float>(surfaceWidth_) - margin));
        }
        if (dy != 0.0f && maxScrollY_ > 0.0f) {
            const float next = std::max(0.0f, std::min(maxScrollY_, scrollY_ + dy));
            if (next != scrollY_) {
                scrollY_ = next;
                pageVertical_.SetValue(scrollY_ / maxScrollY_);
                contentMoved = true;
                changed = true;
            }
        }
        if (dx != 0.0f && maxScrollX_ > 0.0f) {
            const float next = std::max(0.0f, std::min(maxScrollX_, scrollX_ + dx));
            if (next != scrollX_) {
                scrollX_ = next;
                pageHorizontal_.SetValue(scrollX_ / maxScrollX_);
                contentMoved = true;
                changed = true;
            }
        }
        if (changed) {
            TextSelection::MovePointer(selectX_, selectY_);
        }
    }
    const double verticalValue = pageVertical_.GetValue();
    const double horizontalValue = pageHorizontal_.GetValue();
    const bool verticalTicked = pageVertical_.Tick(deltaMs);
    const bool horizontalTicked = pageHorizontal_.Tick(deltaMs);
    if (verticalTicked || horizontalTicked) {
        const float scrollBeforeX = scrollX_;
        const float scrollBeforeY = scrollY_;
        PullPageScroll();
        if (std::fabs(scrollX_ - scrollBeforeX) > 0.5f || std::fabs(scrollY_ - scrollBeforeY) > 0.5f ||
            std::fabs(pageVertical_.GetValue() - verticalValue) > 0.00001 ||
            std::fabs(pageHorizontal_.GetValue() - horizontalValue) > 0.00001) {
            contentMoved = true;
        } else {
            if (verticalTicked) {
                DirtyScrollbar(&pageVertical_);
            }
            if (horizontalTicked) {
                DirtyScrollbar(&pageHorizontal_);
            }
        }
        changed = true;
    }
    if (contentMoved) {
        MarkFullDirty();
    }
    return changed;
}

bool RoboScene::NeedsTick() {
    bool needsTick = false;
    auto visit = [&needsTick](Scrollbar& scrollbar) {
        if (scrollbar.NeedsAnimation()) {
            needsTick = true;
        }
    };
    VisitSceneScrollbars(items_, visit);
    if (pageVertical_.NeedsAnimation() || pageHorizontal_.NeedsAnimation()) {
        needsTick = true;
    }
    if (TextSelection::IsDragging()) {
        const float margin = 24.0f;
        if (selectY_ < margin || selectY_ > static_cast<float>(surfaceHeight_) - margin || selectX_ < margin ||
            selectX_ > static_cast<float>(surfaceWidth_) - margin) {
            needsTick = true;
        }
    }
    if (TextSelection::NeedsBlink()) {
        needsTick = true;
    }
    return needsTick;
}

bool RoboScene::HasFocusedInput() const {
    return focusedInput_ != nullptr;
}

bool RoboScene::SelectableAt(float x, float y) const {
    return TextSelection::Hit(x, y) >= 0;
}

bool RoboScene::HasCaret() const {
    return TextSelection::HasCaret();
}

bool RoboScene::MoveSelection(int command, bool extend) {
    if (focusedInput_) {
        return false;
    }
    float oldX = 0.0f;
    float oldY = 0.0f;
    float oldWidth = 0.0f;
    float oldHeight = 0.0f;
    const bool hadVisual = TextSelection::VisualBounds(oldX, oldY, oldWidth, oldHeight);
    if (!TextSelection::Move(command, extend)) {
        return false;
    }
    if (hadVisual) {
        MarkDirty(oldX, oldY, oldWidth, oldHeight);
    }
    DirtySelectionVisual();
    return true;
}

bool RoboScene::CopySelection() {
    const std::string text = TextSelection::SelectedText();
    if (text.empty() || !platform_) {
        return false;
    }
    platform_->CopyText(text);
    return true;
}

bool RoboScene::SelectAllText() {
    float oldX = 0.0f;
    float oldY = 0.0f;
    float oldWidth = 0.0f;
    float oldHeight = 0.0f;
    const bool hadVisual = TextSelection::VisualBounds(oldX, oldY, oldWidth, oldHeight);
    if (!TextSelection::SelectAll()) {
        return false;
    }
    if (hadVisual) {
        MarkDirty(oldX, oldY, oldWidth, oldHeight);
    }
    DirtySelectionVisual();
    return true;
}

bool RoboScene::KeyInsert(const std::string& utf8) {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->InsertText(utf8)) {
        return false;
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::KeyBackspace() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteBackward()) {
        return false;
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::KeyDelete() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteForward()) {
        return false;
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::KeyLeft() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->MoveCaretLeft()) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyRight() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->MoveCaretRight()) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyEscape() {
    if (focusedInput_) {
        const bool changed = SetFocusedInput(nullptr);
        if (changed) {
            RerenderIfPossible();
        }
        return changed;
    }
    if (focusedScrollbar_) {
        const bool changed = SetFocusedScrollbar(nullptr);
        if (changed) {
            RerenderIfPossible();
        }
        return changed;
    }
    return false;
}

}  // namespace Robo
