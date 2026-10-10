#pragma once

#include "XmlNode.hpp"
#include "Robo/Button.hpp"
#include "Robo/Flexbox.hpp"
#include "Robo/FontEngine.hpp"
#include "Robo/Grid.hpp"
#include "Robo/Heading.hpp"
#include "Robo/IPlatform.hpp"
#include "Robo/Input.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/RoboScene.hpp"
#include "Robo/Scrollbar.hpp"
#include "Robo/Text.hpp"

#include <string>

namespace Robo {

class XmlRuntime {
public:
    XmlRuntime(IPlatform& platform, FontEngine& fonts);

    bool EnsureFonts();
    bool RenderXml(const std::string& xml);
    bool PointerMove(float x, float y);
    bool PointerDown(float x, float y);
    bool PointerUp(float x, float y);
    bool PointerLeave();
    bool HitTest(float x, float y) const;
    HitKind HitTestKind(float x, float y) const;
    bool KeyInsert(const std::string& utf8);
    bool KeyBackspace();
    bool KeyDelete();
    bool KeyLeft();
    bool KeyRight();
    bool KeyEscape();
    bool HasFocusedInput() const;
    const std::string& GetLastError() const;

private:
    void MountNode(const XmlNode& node);
    Text BuildText(const XmlNode& node);
    Heading BuildHeading(const XmlNode& node, int level);
    Paragraph BuildParagraph(const XmlNode& node);
    Button BuildButton(const XmlNode& node);
    Input BuildInput(const XmlNode& node);
    Scrollbar BuildScrollbar(const XmlNode& node);
    Flexbox BuildFlexbox(const XmlNode& node);
    Grid BuildGrid(const XmlNode& node);
    Position BuildPosition(const XmlNode& node);

    void FillTextContent(Text& text, const XmlNode& node, TextStyle style);
    void ApplyTextAttributes(const XmlNode& node, TextStyle& style) const;
    void ApplyButtonAttributes(const XmlNode& node, ButtonStyle& style) const;
    void ApplyInputAttributes(const XmlNode& node, InputStyle& style) const;
    void ApplyScrollbarAttributes(const XmlNode& node, Scrollbar& scrollbar) const;
    void ApplyFlexboxAttributes(const XmlNode& node, FlexboxStyle& style) const;
    void ApplyFlexItemAttributes(const XmlNode& node, FlexItemProps& props) const;
    void ApplyGridAttributes(const XmlNode& node, GridStyle& style) const;
    void ApplyGridItemAttributes(const XmlNode& node, GridItemProps& props) const;
    void ApplyPositionAttributes(const XmlNode& node, PositionStyle& style) const;
    void ApplyPositionItemAttributes(const XmlNode& node, PositionItemProps& props) const;
    void ApplyRunAttributes(const XmlNode& node, TextRun& run) const;
    void CollectRuns(const XmlNode& node, const TextStyle& baseStyle, std::vector<TextRun>& runs);
    std::string CollectPlainText(const XmlNode& node) const;
    int ParseHeadingLevel(const XmlNode& node) const;

    IPlatform& platform_;
    FontEngine& fonts_;
    RoboScene scene_;
    std::string lastError_;
    bool fontsLoaded_ = false;
};

}  // namespace Robo
