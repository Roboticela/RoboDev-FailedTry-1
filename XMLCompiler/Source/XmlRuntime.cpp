#include "XmlRuntime.hpp"
#include "XmlParser.hpp"

#include "Robo/Button.hpp"
#include "Robo/Color.hpp"
#include "Robo/FontStyle.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Input.hpp"
#include "Robo/Paragraph.hpp"
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

void XmlRuntime::FillTextContent(Text& text, const XmlNode& node, TextStyle style) {
    ApplyTextAttributes(node, style);
    text.SetStyle(style);

    std::vector<TextRun> runs;
    CollectRuns(node, style, runs);
    for (const TextRun& run : runs) {
        text.AddRun(run);
    }
}

Text XmlRuntime::BuildText(const XmlNode& node) {
    Text text;
    FillTextContent(text, node, TextStyle{});
    return text;
}

int XmlRuntime::ParseHeadingLevel(const XmlNode& node) const {
    std::string value;
    if (GetAttribute(node, "level", value)) {
        return Heading::ClampLevel(static_cast<int>(ParseDouble(value, 1.0)));
    }

    const std::string& name = node.name;
    if (name.size() == 2 && (name[0] == 'H' || name[0] == 'h') && name[1] >= '1' && name[1] <= '6') {
        return name[1] - '0';
    }
    if (name.size() == 8 && (name.compare(0, 7, "Heading") == 0 || name.compare(0, 7, "heading") == 0) &&
        name[7] >= '1' && name[7] <= '6') {
        return name[7] - '0';
    }
    return 1;
}

Heading XmlRuntime::BuildHeading(const XmlNode& node, int level) {
    Heading heading(level);
    FillTextContent(heading, node, heading.GetStyle());
    return heading;
}

Paragraph XmlRuntime::BuildParagraph(const XmlNode& node) {
    Paragraph paragraph;
    FillTextContent(paragraph, node, paragraph.GetStyle());
    return paragraph;
}

void XmlRuntime::ApplyButtonAttributes(const XmlNode& node, ButtonStyle& style) const {
    std::string value;
    if (GetAttribute(node, "variant", value) || GetAttribute(node, "style", value)) {
        style.variant = ParseButtonVariant(value);
    }
    if (GetAttribute(node, "fontSize", value)) {
        style.fontSize = ParseDouble(value, style.fontSize);
    }
    if (GetAttribute(node, "fontWeight", value)) {
        style.fontWeight = ParseFontWeight(value);
    }
    if (GetAttribute(node, "fontStyle", value)) {
        style.fontStyle = ParseFontStyle(value);
    }
    if (GetAttribute(node, "color", value)) {
        style.textColor = Color::FromHex(value);
        style.hasCustomTextColor = true;
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style.backgroundColor = Color::FromHex(value);
        style.hasCustomBackground = true;
    }
    if (GetAttribute(node, "cornerRadius", value)) {
        style.cornerRadius = ParseDouble(value, style.cornerRadius);
    }
    if (GetAttribute(node, "paddingX", value)) {
        style.paddingX = ParseDouble(value, style.paddingX);
    }
    if (GetAttribute(node, "paddingY", value)) {
        style.paddingY = ParseDouble(value, style.paddingY);
    }
    if (GetAttribute(node, "letterSpacing", value)) {
        style.letterSpacing = ParseDouble(value, style.letterSpacing);
    }
    if (GetAttribute(node, "opacity", value)) {
        style.opacity = ParseDouble(value, style.opacity);
    }
    if (GetAttribute(node, "align", value)) {
        style.align = ParseTextAlign(value);
    }
}

std::string XmlRuntime::CollectPlainText(const XmlNode& node) const {
    std::string content;
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            content += child.text;
        }
    }
    return content;
}

Button XmlRuntime::BuildButton(const XmlNode& node) {
    Button button;
    ButtonStyle style;
    ApplyButtonAttributes(node, style);
    button.SetStyle(style);
    button.ApplyVariantDefaults();

    std::string label;
    if (GetAttribute(node, "title", label) || GetAttribute(node, "label", label)) {
        button.SetLabel(label);
    } else {
        button.SetLabel(CollectPlainText(node));
    }
    return button;
}

void XmlRuntime::ApplyInputAttributes(const XmlNode& node, InputStyle& style) const {
    std::string value;
    if (GetAttribute(node, "variant", value) || GetAttribute(node, "style", value)) {
        style.variant = ParseInputVariant(value);
    }
    if (GetAttribute(node, "fontSize", value)) {
        style.fontSize = ParseDouble(value, style.fontSize);
    }
    if (GetAttribute(node, "fontWeight", value)) {
        style.fontWeight = ParseFontWeight(value);
    }
    if (GetAttribute(node, "fontStyle", value)) {
        style.fontStyle = ParseFontStyle(value);
    }
    if (GetAttribute(node, "color", value)) {
        style.textColor = Color::FromHex(value);
        style.hasCustomTextColor = true;
    }
    if (GetAttribute(node, "placeholderColor", value)) {
        style.placeholderColor = Color::FromHex(value);
        style.hasCustomPlaceholderColor = true;
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style.backgroundColor = Color::FromHex(value);
        style.hasCustomBackground = true;
    }
    if (GetAttribute(node, "borderColor", value)) {
        style.borderColor = Color::FromHex(value);
        style.hasCustomBorder = true;
    }
    if (GetAttribute(node, "focusColor", value) || GetAttribute(node, "focusBorderColor", value)) {
        style.focusBorderColor = Color::FromHex(value);
        style.caretColor = style.focusBorderColor;
    }
    if (GetAttribute(node, "cornerRadius", value)) {
        style.cornerRadius = ParseDouble(value, style.cornerRadius);
    }
    if (GetAttribute(node, "paddingX", value)) {
        style.paddingX = ParseDouble(value, style.paddingX);
    }
    if (GetAttribute(node, "paddingY", value)) {
        style.paddingY = ParseDouble(value, style.paddingY);
    }
    if (GetAttribute(node, "letterSpacing", value)) {
        style.letterSpacing = ParseDouble(value, style.letterSpacing);
    }
    if (GetAttribute(node, "opacity", value)) {
        style.opacity = ParseDouble(value, style.opacity);
    }
    if (GetAttribute(node, "align", value)) {
        style.align = ParseTextAlign(value);
    }
    if (GetAttribute(node, "width", value)) {
        style.width = value;
    }
    if (GetAttribute(node, "minWidth", value)) {
        style.minWidth = ParseDouble(value, style.minWidth);
    }
    if (GetAttribute(node, "maxLength", value)) {
        style.maxLength = static_cast<int>(ParseDouble(value, style.maxLength));
    }
}

Input XmlRuntime::BuildInput(const XmlNode& node) {
    Input input;
    InputStyle style;
    ApplyInputAttributes(node, style);
    input.SetStyle(style);
    input.ApplyVariantDefaults();

    std::string placeholder;
    if (GetAttribute(node, "placeholder", placeholder) || GetAttribute(node, "hint", placeholder)) {
        input.SetPlaceholder(placeholder);
    }

    std::string value;
    if (GetAttribute(node, "value", value) || GetAttribute(node, "text", value)) {
        input.SetValue(value);
    } else {
        input.SetValue(CollectPlainText(node));
    }
    input.SetCaretIndex(input.GetValue().size());
    return input;
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

    if (node.name == "Heading" || node.name == "heading" ||
        (node.name.size() == 2 && (node.name[0] == 'H' || node.name[0] == 'h') && node.name[1] >= '1' &&
         node.name[1] <= '6') ||
        (node.name.size() == 8 && (node.name.compare(0, 7, "Heading") == 0 || node.name.compare(0, 7, "heading") == 0) &&
         node.name[7] >= '1' && node.name[7] <= '6')) {
        scene_.AddText(BuildHeading(node, ParseHeadingLevel(node)));
        return;
    }

    if (node.name == "Paragraph" || node.name == "paragraph" || node.name == "P" || node.name == "p") {
        scene_.AddText(BuildParagraph(node));
        return;
    }

    if (node.name == "Button") {
        scene_.AddButton(BuildButton(node));
        return;
    }

    if (node.name == "Input" || node.name == "TextField" || node.name == "TextInput") {
        scene_.AddInput(BuildInput(node));
        return;
    }

    lastError_ = "Unsupported element: <" + node.name +
                 ">. Available: <Robo>, <Text>, <Heading>/<H1-H6>, <Paragraph>, <Button>, <Input>.";
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

bool XmlRuntime::PointerMove(float x, float y) {
    return scene_.PointerMove(x, y);
}

bool XmlRuntime::PointerDown(float x, float y) {
    return scene_.PointerDown(x, y);
}

bool XmlRuntime::PointerUp(float x, float y) {
    return scene_.PointerUp(x, y);
}

bool XmlRuntime::PointerLeave() {
    return scene_.PointerLeave();
}

HitKind XmlRuntime::HitTestKind(float x, float y) const {
    return scene_.HitTestKind(x, y);
}

bool XmlRuntime::KeyInsert(const std::string& utf8) {
    return scene_.KeyInsert(utf8);
}

bool XmlRuntime::KeyBackspace() {
    return scene_.KeyBackspace();
}

bool XmlRuntime::KeyDelete() {
    return scene_.KeyDelete();
}

bool XmlRuntime::KeyLeft() {
    return scene_.KeyLeft();
}

bool XmlRuntime::KeyRight() {
    return scene_.KeyRight();
}

bool XmlRuntime::KeyEscape() {
    return scene_.KeyEscape();
}

bool XmlRuntime::HasFocusedInput() const {
    return scene_.HasFocusedInput();
}

bool XmlRuntime::HitTest(float x, float y) const {
    return scene_.HitTest(x, y);
}

}  // namespace Robo
