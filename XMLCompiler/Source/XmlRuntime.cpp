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

void XmlRuntime::ApplyFlexboxAttributes(const XmlNode& node, FlexboxStyle& style) const {
    std::string value;
    if (GetAttribute(node, "direction", value) || GetAttribute(node, "flexDirection", value)) {
        style.direction = ParseFlexDirection(value);
    }
    if (GetAttribute(node, "justify", value) || GetAttribute(node, "justifyContent", value)) {
        style.justify = ParseFlexJustify(value);
    }
    if (GetAttribute(node, "align", value) || GetAttribute(node, "alignItems", value)) {
        style.align = ParseFlexAlign(value);
    }
    if (GetAttribute(node, "wrap", value) || GetAttribute(node, "flexWrap", value)) {
        style.wrap = ParseFlexWrap(value);
    }
    if (GetAttribute(node, "gap", value)) {
        style.gap = static_cast<float>(ParseDouble(value, style.gap));
    }
    if (GetAttribute(node, "gapX", value) || GetAttribute(node, "columnGap", value)) {
        style.columnGap = static_cast<float>(ParseDouble(value, style.columnGap));
    }
    if (GetAttribute(node, "gapY", value) || GetAttribute(node, "rowGap", value)) {
        style.rowGap = static_cast<float>(ParseDouble(value, style.rowGap));
    }
    if (GetAttribute(node, "width", value)) {
        style.width = value;
    }
    if (GetAttribute(node, "height", value)) {
        style.height = value;
    }
    if (GetAttribute(node, "minWidth", value)) {
        style.minWidth = static_cast<float>(ParseDouble(value, style.minWidth));
    }
    if (GetAttribute(node, "maxWidth", value)) {
        style.maxWidth = static_cast<float>(ParseDouble(value, style.maxWidth));
    }
    if (GetAttribute(node, "minHeight", value)) {
        style.minHeight = static_cast<float>(ParseDouble(value, style.minHeight));
    }
    if (GetAttribute(node, "maxHeight", value)) {
        style.maxHeight = static_cast<float>(ParseDouble(value, style.maxHeight));
    }
    if (GetAttribute(node, "padding", value)) {
        const float p = static_cast<float>(ParseDouble(value, 0.0));
        style.paddingTop = p;
        style.paddingBottom = p;
        style.paddingLeft = p;
        style.paddingRight = p;
    }
    if (GetAttribute(node, "paddingX", value)) {
        const float px = static_cast<float>(ParseDouble(value, 0.0));
        style.paddingLeft = px;
        style.paddingRight = px;
    }
    if (GetAttribute(node, "paddingY", value)) {
        const float py = static_cast<float>(ParseDouble(value, 0.0));
        style.paddingTop = py;
        style.paddingBottom = py;
    }
    if (GetAttribute(node, "paddingTop", value)) {
        style.paddingTop = static_cast<float>(ParseDouble(value, style.paddingTop));
    }
    if (GetAttribute(node, "paddingBottom", value)) {
        style.paddingBottom = static_cast<float>(ParseDouble(value, style.paddingBottom));
    }
    if (GetAttribute(node, "paddingLeft", value)) {
        style.paddingLeft = static_cast<float>(ParseDouble(value, style.paddingLeft));
    }
    if (GetAttribute(node, "paddingRight", value)) {
        style.paddingRight = static_cast<float>(ParseDouble(value, style.paddingRight));
    }
    if (GetAttribute(node, "margin", value)) {
        const float m = static_cast<float>(ParseDouble(value, 0.0));
        style.marginTop = m;
        style.marginBottom = m;
        style.marginLeft = m;
        style.marginRight = m;
    }
    if (GetAttribute(node, "marginX", value)) {
        const float mx = static_cast<float>(ParseDouble(value, 0.0));
        style.marginLeft = mx;
        style.marginRight = mx;
    }
    if (GetAttribute(node, "marginY", value)) {
        const float my = static_cast<float>(ParseDouble(value, 0.0));
        style.marginTop = my;
        style.marginBottom = my;
    }
    if (GetAttribute(node, "marginTop", value)) {
        style.marginTop = static_cast<float>(ParseDouble(value, style.marginTop));
    }
    if (GetAttribute(node, "marginBottom", value)) {
        style.marginBottom = static_cast<float>(ParseDouble(value, style.marginBottom));
    }
    if (GetAttribute(node, "marginLeft", value)) {
        style.marginLeft = static_cast<float>(ParseDouble(value, style.marginLeft));
    }
    if (GetAttribute(node, "marginRight", value)) {
        style.marginRight = static_cast<float>(ParseDouble(value, style.marginRight));
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style.backgroundColor = Color::FromHex(value);
    }
    if (GetAttribute(node, "borderColor", value)) {
        style.borderColor = Color::FromHex(value);
    }
    if (GetAttribute(node, "borderWidth", value)) {
        style.borderWidth = static_cast<float>(ParseDouble(value, style.borderWidth));
    }
    if (GetAttribute(node, "cornerRadius", value) || GetAttribute(node, "borderRadius", value)) {
        style.cornerRadius = static_cast<float>(ParseDouble(value, style.cornerRadius));
    }
    if (GetAttribute(node, "opacity", value)) {
        style.opacity = static_cast<float>(ParseDouble(value, style.opacity));
    }
}

void XmlRuntime::ApplyFlexItemAttributes(const XmlNode& node, FlexItemProps& props) const {
    std::string value;
    if (GetAttribute(node, "flex", value)) {
        props.flexGrow = static_cast<float>(ParseDouble(value, props.flexGrow));
    }
    if (GetAttribute(node, "flexGrow", value)) {
        props.flexGrow = static_cast<float>(ParseDouble(value, props.flexGrow));
    }
    if (GetAttribute(node, "flexShrink", value)) {
        props.flexShrink = static_cast<float>(ParseDouble(value, props.flexShrink));
    }
    if (GetAttribute(node, "flexBasis", value)) {
        if (!value.empty() && value.back() == '%') {
            props.flexBasisString = value;
        } else {
            props.flexBasis = static_cast<float>(ParseDouble(value, -1.0));
        }
    }
    if (GetAttribute(node, "alignSelf", value)) {
        props.alignSelf = ParseFlexAlign(value);
    }
    if (GetAttribute(node, "width", value)) {
        props.width = value;
    }
    if (GetAttribute(node, "height", value)) {
        props.height = value;
    }
    if (GetAttribute(node, "margin", value)) {
        const float m = static_cast<float>(ParseDouble(value, 0.0));
        props.marginTop = m;
        props.marginBottom = m;
        props.marginLeft = m;
        props.marginRight = m;
    }
    if (GetAttribute(node, "marginTop", value)) {
        props.marginTop = static_cast<float>(ParseDouble(value, props.marginTop));
    }
    if (GetAttribute(node, "marginBottom", value)) {
        props.marginBottom = static_cast<float>(ParseDouble(value, props.marginBottom));
    }
    if (GetAttribute(node, "marginLeft", value)) {
        props.marginLeft = static_cast<float>(ParseDouble(value, props.marginLeft));
    }
    if (GetAttribute(node, "marginRight", value)) {
        props.marginRight = static_cast<float>(ParseDouble(value, props.marginRight));
    }
}

Flexbox XmlRuntime::BuildFlexbox(const XmlNode& node) {
    Flexbox flexbox;
    FlexboxStyle style;
    ApplyFlexboxAttributes(node, style);
    flexbox.SetStyle(style);

    for (const XmlNode& child : node.children) {
        if (child.isText) {
            std::string content = child.text;
            bool hasContent = false;
            for (char c : content) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    hasContent = true;
                    break;
                }
            }
            if (hasContent) {
                Text text;
                TextStyle ts;
                ts.color = Color::FromHex("#1C1C1E");
                text.SetStyle(ts);
                text.AddPlain(content);
                flexbox.AddText(std::move(text));
            }
            continue;
        }

        FlexItemProps props;
        ApplyFlexItemAttributes(child, props);

        if (child.name == "Text") {
            flexbox.AddText(BuildText(child), props);
        } else if (child.name == "Heading" || child.name == "heading" ||
                   (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' &&
                    child.name[1] <= '6') ||
                   (child.name.size() == 8 &&
                    (child.name.compare(0, 7, "Heading") == 0 || child.name.compare(0, 7, "heading") == 0) &&
                    child.name[7] >= '1' && child.name[7] <= '6')) {
            flexbox.AddText(BuildHeading(child, ParseHeadingLevel(child)), props);
        } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
            flexbox.AddText(BuildParagraph(child), props);
        } else if (child.name == "Button") {
            flexbox.AddButton(BuildButton(child), props);
        } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
            flexbox.AddInput(BuildInput(child), props);
        } else if (child.name == "Flexbox" || child.name == "flexbox" || child.name == "Flex" || child.name == "flex") {
            flexbox.AddFlexbox(BuildFlexbox(child), props);
        }
    }

    return flexbox;
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

    if (node.name == "Flexbox" || node.name == "flexbox" || node.name == "Flex" || node.name == "flex") {
        scene_.AddFlexbox(BuildFlexbox(node));
        return;
    }

    lastError_ = "Unsupported element: <" + node.name +
                 ">. Available: <Robo>, <Text>, <Heading>/<H1-H6>, <Paragraph>, <Button>, <Input>, <Flexbox>.";
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
