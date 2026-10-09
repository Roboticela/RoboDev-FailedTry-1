#pragma once

#include "XmlNode.hpp"
#include "Robo/Button.hpp"
#include "Robo/FontEngine.hpp"
#include "Robo/Heading.hpp"
#include "Robo/IPlatform.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/RoboScene.hpp"
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
    const std::string& GetLastError() const;

private:
    void MountNode(const XmlNode& node);
    Text BuildText(const XmlNode& node);
    Heading BuildHeading(const XmlNode& node, int level);
    Paragraph BuildParagraph(const XmlNode& node);
    Button BuildButton(const XmlNode& node);
    void FillTextContent(Text& text, const XmlNode& node, TextStyle style);
    void ApplyTextAttributes(const XmlNode& node, TextStyle& style) const;
    void ApplyButtonAttributes(const XmlNode& node, ButtonStyle& style) const;
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
