#pragma once

#include "Button.hpp"
#include "Color.hpp"
#include "Input.hpp"
#include "Scrollbar.hpp"
#include "Text.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Robo {

class Flexbox;
class Grid;

enum class GridTrackType {
    Fraction,
    Pixel,
    Percent,
    Auto
};

struct GridTrack {
    GridTrackType type = GridTrackType::Fraction;
    float value = 1.0f;
};

enum class GridAutoFlow {
    Row,
    Column
};

enum class GridAlignment {
    Stretch,
    Start,
    Center,
    End,
    Auto
};

enum class GridJustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class GridItemKind {
    Text,
    Button,
    Input,
    Scrollbar,
    Flexbox,
    Grid
};

struct GridStyle {
    std::vector<GridTrack> templateColumns;
    std::vector<GridTrack> templateRows;
    int columnCount = 0;
    int rowCount = 0;
    std::string columnsString;
    std::string rowsString;

    float gap = 0.0f;
    float columnGap = 0.0f;
    float rowGap = 0.0f;

    GridAutoFlow autoFlow = GridAutoFlow::Row;
    GridAlignment justifyItems = GridAlignment::Stretch;
    GridAlignment alignItems = GridAlignment::Stretch;
    GridJustifyContent justifyContent = GridJustifyContent::Start;

    std::string width = "auto";
    std::string height = "auto";
    float minWidth = 0.0f;
    float maxWidth = 100000.0f;
    float minHeight = 0.0f;
    float maxHeight = 100000.0f;

    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;

    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;

    Color backgroundColor = Color::FromRgba(0, 0, 0, 0);
    Color borderColor = Color::FromRgba(0, 0, 0, 0);
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    float opacity = 1.0f;
};

struct GridItemProps {
    int column = 0;
    int row = 0;
    int columnSpan = 1;
    int rowSpan = 1;

    GridAlignment justifySelf = GridAlignment::Auto;
    GridAlignment alignSelf = GridAlignment::Auto;

    std::string width;
    std::string height;
    std::string minWidth;
    std::string maxWidth;
    std::string minHeight;
    std::string maxHeight;

    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
};

struct GridBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

struct GridItem {
    GridItemKind kind = GridItemKind::Text;
    GridItemProps props;
    Text text;
    Button button;
    Input input;
    Scrollbar scrollbar;
    std::shared_ptr<Flexbox> flexbox;
    std::shared_ptr<Grid> grid;
};

class Grid {
public:
    Grid();

    void SetStyle(const GridStyle& style);
    const GridStyle& GetStyle() const;
    GridStyle& GetStyle();

    void AddText(Text text, const GridItemProps& props = {});
    void AddButton(Button button, const GridItemProps& props = {});
    void AddInput(Input input, const GridItemProps& props = {});
    void AddScrollbar(Scrollbar scrollbar, const GridItemProps& props = {});
    void AddFlexbox(Flexbox flexbox, const GridItemProps& props = {});
    void AddGrid(Grid grid, const GridItemProps& props = {});
    void AddItem(GridItem item);

    const std::vector<GridItem>& GetItems() const;
    std::vector<GridItem>& GetItems();

    void SetBounds(const GridBounds& bounds);
    const GridBounds& GetBounds() const;

    float ResolveGapX(float pixelScale) const;
    float ResolveGapY(float pixelScale) const;

private:
    GridStyle style_;
    std::vector<GridItem> items_;
    GridBounds bounds_;
};

std::vector<GridTrack> ParseGridTracks(const std::string& value);
GridAutoFlow ParseGridAutoFlow(const std::string& value);
GridAlignment ParseGridAlignment(const std::string& value);
GridJustifyContent ParseGridJustifyContent(const std::string& value);

}  // namespace Robo
