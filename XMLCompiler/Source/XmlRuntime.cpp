#include "XmlRuntime.hpp"
#include "XmlParser.hpp"

#include "Robo/Color.hpp"
#include "Robo/FontStyle.hpp"
#include "Robo/TextAlign.hpp"

#include <cstdlib>

namespace Robo {

namespace {

bool GetAttribute(const XmlNode& node, const std::string& key, std::string& value) {
    const auto it = node.attributes.find(key);
    if (it == node.attributes.end()) {
        return false;
    }
    value = it->second;
    return true;
}

double ParseDouble(const std::string& value, double fallback) {
    if (value.empty()) {
        return fallback;
    }
    char* end = nullptr;
    const double parsed = std::strtod(value.c_str(), &end);
    if (end == value.c_str()) {
        return fallback;
    }
    return parsed;
}

bool ParseBool(const std::string& value, bool fallback) {
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    return fallback;
}

}  // namespace

XmlRuntime::XmlRuntime(IPlatform& platform, FontEngine& fonts) : platform_(platform), fonts_(fonts) {}

const std::string& XmlRuntime::GetLastError() const {
    return lastError_;
}

bool XmlRuntime::EnsureFonts() {
    if (fontsLoaded_ && fonts_.IsReady()) {
        return true;
    }

    const bool regular = fonts_.LoadFace(FontFaceId::Regular, platform_.LoadAsset("Assets/Fonts/Inter-Regular.ttf"));
    fonts_.LoadFace(FontFaceId::Medium, platform_.LoadAsset("Assets/Fonts/Inter-Medium.ttf"));
    fonts_.LoadFace(FontFaceId::SemiBold, platform_.LoadAsset("Assets/Fonts/Inter-SemiBold.ttf"));
    fonts_.LoadFace(FontFaceId::Italic, platform_.LoadAsset("Assets/Fonts/Inter-Italic.ttf"));

    fontsLoaded_ = regular;
    if (!fontsLoaded_) {
        lastError_ = "Failed to load Inter-Regular.ttf from Assets/Fonts";
    }
    return fontsLoaded_;
}

void XmlRuntime::ApplyTextAttributes(const XmlNode& node, TextStyle& style) const {
    std::string value;
    if (GetAttribute(node, "fontSize", value)) {
        style.fontSize = ParseDouble(value, style.fontSize);
    }
    if (GetAttribute(node, "color", value)) {
        style.color = Color::FromHex(value);
    }
    if (GetAttribute(node, "fontWeight", value)) {
        style.fontWeight = ParseFontWeight(value);
    }
    if (GetAttribute(node, "fontStyle", value)) {
        style.fontStyle = ParseFontStyle(value);
    }
    if (GetAttribute(node, "textDecoration", value)) {
        style.textDecoration = ParseTextDecoration(value);
    }
    if (GetAttribute(node, "align", value)) {
        style.align = ParseTextAlign(value);
    }
    if (GetAttribute(node, "wrap", value)) {
        style.wrap = ParseBool(value, style.wrap);
    }
    if (GetAttribute(node, "lineHeight", value)) {
        style.lineHeight = ParseDouble(value, style.lineHeight);
    }
    if (GetAttribute(node, "letterSpacing", value)) {
        style.letterSpacing = ParseDouble(value, style.letterSpacing);
    }
    if (GetAttribute(node, "opacity", value)) {
        style.opacity = ParseDouble(value, style.opacity);
    }
    if (GetAttribute(node, "width", value)) {
        style.width = value;
    }
    if (GetAttribute(node, "maxWidth", value)) {
        style.maxWidth = value;
    }
}

void XmlRuntime::ApplyRunAttributes(const XmlNode& node, TextRun& run) const {
    std::string value;
    if (GetAttribute(node, "fontSize", value)) {
        run.fontSize = ParseDouble(value, run.fontSize);
        run.hasFontSize = true;
    }
    if (GetAttribute(node, "color", value)) {
        run.color = Color::FromHex(value);
        run.hasColor = true;
    }
    if (GetAttribute(node, "fontWeight", value)) {
        run.fontWeight = ParseFontWeight(value);
        run.hasFontWeight = true;
    }
    if (GetAttribute(node, "fontStyle", value)) {
        run.fontStyle = ParseFontStyle(value);
        run.hasFontStyle = true;
    }
    if (GetAttribute(node, "textDecoration", value)) {
        run.textDecoration = ParseTextDecoration(value);
        run.hasTextDecoration = true;
    }
    if (GetAttribute(node, "letterSpacing", value)) {
        run.letterSpacing = ParseDouble(value, run.letterSpacing);
        run.hasLetterSpacing = true;
    }
    if (GetAttribute(node, "opacity", value)) {
        run.opacity = ParseDouble(value, run.opacity);
        run.hasOpacity = true;
    }
}

void XmlRuntime::CollectRuns(const XmlNode& node, const TextStyle& baseStyle, std::vector<TextRun>& runs) {
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            TextRun run;
            run.content = child.text;
            run.fontSize = baseStyle.fontSize;
            run.color = baseStyle.color;
            run.fontWeight = baseStyle.fontWeight;
            run.fontStyle = baseStyle.fontStyle;
            run.textDecoration = baseStyle.textDecoration;
            run.letterSpacing = baseStyle.letterSpacing;
            run.opacity = baseStyle.opacity;
            runs.push_back(run);
            continue;
        }

        if (child.name == "Span" || child.name == "span") {
            TextRun run;
            run.fontSize = baseStyle.fontSize;
            run.color = baseStyle.color;
            run.fontWeight = baseStyle.fontWeight;
            run.fontStyle = baseStyle.fontStyle;
            run.textDecoration = baseStyle.textDecoration;
            run.letterSpacing = baseStyle.letterSpacing;
            run.opacity = baseStyle.opacity;
            ApplyRunAttributes(child, run);

            std::string content;
            for (const XmlNode& spanChild : child.children) {
                if (spanChild.isText) {
                    content += spanChild.text;
                }
            }
            run.content = content;
            runs.push_back(run);
            continue;
        }

        lastError_ = "Unsupported child inside Text: <" + child.name + ">";
    }
}

Text XmlRuntime::BuildText(const XmlNode& node) {
    Text text;
    TextStyle style;
    ApplyTextAttributes(node, style);
    text.SetStyle(style);

    std::vector<TextRun> runs;
    CollectRuns(node, style, runs);
    for (const TextRun& run : runs) {
        text.AddRun(run);
    }
    return text;
}

void XmlRuntime::MountNode(const XmlNode& node) {
    if (node.isText) {
        return;
    }

    if (node.name == "Robo") {
        for (const XmlNode& child : node.children) {
            MountNode(child);
        }
        return;
    }

    if (node.name == "Text") {
        scene_.AddText(BuildText(node));
        return;
    }

    lastError_ = "Unsupported element: <" + node.name + ">. Only <Robo> and <Text> are available until Screen is implemented.";
}

bool XmlRuntime::RenderXml(const std::string& xml) {
    lastError_.clear();
    scene_.Clear();

    if (!EnsureFonts()) {
        return false;
    }

    try {
        XmlParser parser;
        const XmlNode root = parser.Parse(xml);
        MountNode(root);
        if (!lastError_.empty()) {
            return false;
        }
        scene_.Render(platform_, fonts_, Color::FromHex("#F7F3EB"), 28.0f);
        return true;
    } catch (const std::exception& exception) {
        lastError_ = exception.what();
        return false;
    }
}

}  // namespace Robo
