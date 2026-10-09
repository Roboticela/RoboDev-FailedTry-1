#include "HtmlFromXml.hpp"
#include "XmlParser.hpp"

#include "Robo/Color.hpp"
#include "Robo/FontStyle.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/TextAlign.hpp"

#include <cstdlib>
#include <sstream>

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

}  // namespace

std::string HtmlFromXml::EscapeHtml(const std::string& value) const {
    std::string result;
    result.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            default:
                result.push_back(ch);
                break;
        }
    }
    return result;
}

std::string HtmlFromXml::StyleFromAttributes(const XmlNode& node, bool isBlock) const {
    std::ostringstream style;
    std::string value;

    style << "font-family:'Inter',sans-serif;";

    if (GetAttribute(node, "fontSize", value)) {
        style << "font-size:" << value << "px;";
    }
    if (GetAttribute(node, "color", value)) {
        style << "color:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "fontWeight", value)) {
        style << "font-weight:" << FontWeightToCss(ParseFontWeight(value)) << ";";
    }
    if (GetAttribute(node, "fontStyle", value)) {
        style << "font-style:" << FontStyleToCss(ParseFontStyle(value)) << ";";
    }
    if (GetAttribute(node, "textDecoration", value)) {
        style << "text-decoration:" << TextDecorationToCss(ParseTextDecoration(value)) << ";";
    }
    if (GetAttribute(node, "letterSpacing", value)) {
        style << "letter-spacing:" << value << "px;";
    }
    if (GetAttribute(node, "opacity", value)) {
        style << "opacity:" << value << ";";
    }

    if (isBlock) {
        if (GetAttribute(node, "align", value)) {
            style << "text-align:" << TextAlignToCss(ParseTextAlign(value)) << ";";
        }
        if (GetAttribute(node, "wrap", value)) {
            if (value == "false" || value == "0" || value == "no") {
                style << "white-space:nowrap;overflow:hidden;text-overflow:ellipsis;";
            } else {
                style << "white-space:normal;overflow-wrap:anywhere;";
            }
        } else {
            style << "white-space:normal;overflow-wrap:anywhere;";
        }
        if (GetAttribute(node, "lineHeight", value)) {
            style << "line-height:" << value << ";";
        }
        if (GetAttribute(node, "width", value)) {
            style << "width:" << value << ";";
        }
        if (GetAttribute(node, "maxWidth", value)) {
            style << "max-width:" << value << ";";
        }
        style << "margin:0 0 0.75rem 0;";
    }

    return style.str();
}

std::string HtmlFromXml::BuildSpanHtml(const XmlNode& node) const {
    std::ostringstream html;
    html << "<span style=\"" << StyleFromAttributes(node, false) << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
        }
    }
    html << "</span>";
    return html.str();
}

int HtmlFromXml::ParseHeadingLevel(const XmlNode& node) const {
    std::string value;
    if (GetAttribute(node, "level", value)) {
        const int level = static_cast<int>(std::atof(value.c_str()));
        if (level >= 1 && level <= 6) {
            return level;
        }
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

std::string HtmlFromXml::BuildTextHtml(const XmlNode& node, const std::string& tagName) const {
    std::ostringstream html;
    html << "<" << tagName << " style=\"";

    if (!tagName.empty() && tagName[0] == 'h' && tagName.size() == 2 && tagName[1] >= '1' && tagName[1] <= '6') {
        const TextStyle defaults = Heading::DefaultStyle(tagName[1] - '0');
        html << "font-family:'Inter',sans-serif;"
             << "font-size:" << defaults.fontSize << "px;"
             << "font-weight:" << FontWeightToCss(defaults.fontWeight) << ";"
             << "line-height:" << defaults.lineHeight << ";"
             << "color:" << defaults.color.ToCss() << ";"
             << "margin:0 0 0.75rem 0;";
    } else if (tagName == "p") {
        const TextStyle defaults = Paragraph::DefaultStyle();
        html << "font-family:'Inter',sans-serif;"
             << "font-size:" << defaults.fontSize << "px;"
             << "font-weight:" << FontWeightToCss(defaults.fontWeight) << ";"
             << "line-height:" << defaults.lineHeight << ";"
             << "color:" << defaults.color.ToCss() << ";"
             << "max-width:" << defaults.maxWidth << ";"
             << "margin:0 0 0.75rem 0;";
    }

    html << StyleFromAttributes(node, true) << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
        } else if (child.name == "Span" || child.name == "span") {
            html << BuildSpanHtml(child);
        }
    }
    html << "</" << tagName << ">";
    return html.str();
}

std::string HtmlFromXml::BuildButtonHtml(const XmlNode& node) const {
    std::string label;
    if (!GetAttribute(node, "title", label)) {
        GetAttribute(node, "label", label);
    }
    if (label.empty()) {
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                label += child.text;
            }
        }
    }

    std::string variant = "filled";
    if (!GetAttribute(node, "variant", variant)) {
        GetAttribute(node, "style", variant);
    }

    std::string background = "#007AFF";
    std::string color = "#FFFFFF";
    if (variant == "tinted") {
        background = "#E5F1FF";
        color = "#007AFF";
    } else if (variant == "gray" || variant == "grey") {
        background = "#E5E5EA";
        color = "#1C1C1E";
    } else if (variant == "plain") {
        background = "transparent";
        color = "#007AFF";
    }

    std::string custom;
    if (GetAttribute(node, "background", custom) || GetAttribute(node, "backgroundColor", custom)) {
        background = custom;
    }
    if (GetAttribute(node, "color", custom)) {
        color = custom;
    }

    std::string align = "left";
    GetAttribute(node, "align", align);
    std::string fontSize = "17";
    GetAttribute(node, "fontSize", fontSize);
    std::string cornerRadius = "14";
    GetAttribute(node, "cornerRadius", cornerRadius);

    std::ostringstream html;
    html << "<button type=\"button\" style=\""
         << "display:inline-block;"
         << "font-family:'Inter',sans-serif;"
         << "font-size:" << fontSize << "px;"
         << "font-weight:600;"
         << "line-height:1.2;"
         << "white-space:nowrap;"
         << "border:none;"
         << "border-radius:" << cornerRadius << "px;"
         << "padding:12px 20px;"
         << "background:" << background << ";"
         << "color:" << color << ";"
         << "margin:0 0 16px 0;";
    if (align == "center") {
        html << "display:block;margin-left:auto;margin-right:auto;";
    } else if (align == "right") {
        html << "display:block;margin-left:auto;";
    }
    html << "\">" << EscapeHtml(label) << "</button>";
    return html.str();
}

std::string HtmlFromXml::BuildInputHtml(const XmlNode& node) const {
    std::string placeholder;
    GetAttribute(node, "placeholder", placeholder);
    if (placeholder.empty()) {
        GetAttribute(node, "hint", placeholder);
    }

    std::string value;
    if (!GetAttribute(node, "value", value)) {
        GetAttribute(node, "text", value);
    }
    if (value.empty()) {
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                value += child.text;
            }
        }
    }

    std::string variant = "filled";
    if (!GetAttribute(node, "variant", variant)) {
        GetAttribute(node, "style", variant);
    }

    std::string background = "#F2F2F7";
    std::string border = "none";
    std::string radius = "10";
    if (variant == "bordered" || variant == "border") {
        background = "#FFFFFF";
        border = "1px solid #C6C6C8";
        radius = "8";
    } else if (variant == "plain") {
        background = "transparent";
        border = "none";
        radius = "0";
    }

    std::string custom;
    if (GetAttribute(node, "background", custom) || GetAttribute(node, "backgroundColor", custom)) {
        background = custom;
    }
    if (GetAttribute(node, "cornerRadius", custom)) {
        radius = custom;
    }

    std::string align = "left";
    GetAttribute(node, "align", align);
    std::string fontSize = "17";
    GetAttribute(node, "fontSize", fontSize);
    std::string width = "100%";
    GetAttribute(node, "width", width);

    std::ostringstream html;
    html << "<input type=\"text\" placeholder=\"" << EscapeHtml(placeholder) << "\" value=\""
         << EscapeHtml(value) << "\" style=\""
         << "box-sizing:border-box;"
         << "width:" << width << ";"
         << "font-family:'Inter',sans-serif;"
         << "font-size:" << fontSize << "px;"
         << "font-weight:400;"
         << "line-height:1.2;"
         << "color:#1C1C1E;"
         << "background:" << background << ";";
    if (variant == "plain") {
        html << "border:none;border-bottom:1px solid #C6C6C8;border-radius:0;";
    } else {
        html << "border:" << border << ";"
             << "border-radius:" << radius << "px;";
    }
    html << "padding:10px 12px;"
         << "outline-color:#007AFF;"
         << "margin:0 0 16px 0;";
    if (align == "center") {
        html << "display:block;margin-left:auto;margin-right:auto;";
    } else if (align == "right") {
        html << "display:block;margin-left:auto;";
    }
    html << "\" />";
    return html.str();
}

std::string HtmlFromXml::ConvertNode(const XmlNode& node) const {
    if (node.isText) {
        return EscapeHtml(node.text);
    }

    if (node.name == "Robo") {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
             << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
             << "<title>Robo Page</title>"
             << "<style>@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Regular.ttf');font-weight:400;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Medium.ttf');font-weight:500;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-SemiBold.ttf');font-weight:600;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Italic.ttf');font-style:italic;font-weight:400;}"
             << "body{margin:0;padding:24px;background:#FFFFFF;color:#1A1A1A;font-family:'Inter',sans-serif;}</style>"
             << "</head><body>";
        for (const XmlNode& child : node.children) {
            html << ConvertNode(child);
        }
        html << "</body></html>";
        return html.str();
    }

    if (node.name == "Text") {
        return BuildTextHtml(node, "p");
    }

    if (node.name == "Heading" || node.name == "heading" ||
        (node.name.size() == 2 && (node.name[0] == 'H' || node.name[0] == 'h') && node.name[1] >= '1' &&
         node.name[1] <= '6') ||
        (node.name.size() == 8 && (node.name.compare(0, 7, "Heading") == 0 || node.name.compare(0, 7, "heading") == 0) &&
         node.name[7] >= '1' && node.name[7] <= '6')) {
        return BuildTextHtml(node, "h" + std::to_string(ParseHeadingLevel(node)));
    }

    if (node.name == "Paragraph" || node.name == "paragraph" || node.name == "P" || node.name == "p") {
        return BuildTextHtml(node, "p");
    }

    if (node.name == "Button") {
        return BuildButtonHtml(node);
    }

    if (node.name == "Input" || node.name == "TextField" || node.name == "TextInput") {
        return BuildInputHtml(node);
    }

    if (node.name == "Flexbox" || node.name == "flexbox" || node.name == "Flex" || node.name == "flex") {
        return BuildFlexboxHtml(node);
    }

    return "";
}

std::string HtmlFromXml::BuildFlexboxHtml(const XmlNode& node) const {
    std::ostringstream style;
    style << "display:flex;box-sizing:border-box;";

    std::string value;
    if (GetAttribute(node, "direction", value) || GetAttribute(node, "flexDirection", value)) {
        if (value == "column" || value == "col") {
            style << "flex-direction:column;";
        } else if (value == "column-reverse" || value == "col-reverse") {
            style << "flex-direction:column-reverse;";
        } else if (value == "row-reverse") {
            style << "flex-direction:row-reverse;";
        } else {
            style << "flex-direction:row;";
        }
    } else {
        style << "flex-direction:row;";
    }

    if (GetAttribute(node, "justify", value) || GetAttribute(node, "justifyContent", value)) {
        if (value == "center") {
            style << "justify-content:center;";
        } else if (value == "flex-end" || value == "end") {
            style << "justify-content:flex-end;";
        } else if (value == "space-between" || value == "between") {
            style << "justify-content:space-between;";
        } else if (value == "space-around" || value == "around") {
            style << "justify-content:space-around;";
        } else if (value == "space-evenly" || value == "evenly") {
            style << "justify-content:space-evenly;";
        } else {
            style << "justify-content:flex-start;";
        }
    }
    if (GetAttribute(node, "align", value) || GetAttribute(node, "alignItems", value)) {
        if (value == "flex-start" || value == "start") {
            style << "align-items:flex-start;";
        } else if (value == "center") {
            style << "align-items:center;";
        } else if (value == "flex-end" || value == "end") {
            style << "align-items:flex-end;";
        } else {
            style << "align-items:stretch;";
        }
    }
    if (GetAttribute(node, "wrap", value) || GetAttribute(node, "flexWrap", value)) {
        if (value == "wrap") {
            style << "flex-wrap:wrap;";
        } else if (value == "wrap-reverse") {
            style << "flex-wrap:wrap-reverse;";
        } else {
            style << "flex-wrap:nowrap;";
        }
    }
    if (GetAttribute(node, "gap", value)) {
        style << "gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "gapX", value) || GetAttribute(node, "columnGap", value)) {
        style << "column-gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "gapY", value) || GetAttribute(node, "rowGap", value)) {
        style << "row-gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "width", value)) {
        style << "width:" << value << (value.find('%') == std::string::npos && value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "height", value)) {
        style << "height:" << value << (value.find('%') == std::string::npos && value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "minWidth", value)) {
        style << "min-width:" << value << "px;";
    }
    if (GetAttribute(node, "maxWidth", value)) {
        style << "max-width:" << value << "px;";
    }
    if (GetAttribute(node, "padding", value)) {
        style << "padding:" << value << "px;";
    }
    if (GetAttribute(node, "paddingX", value)) {
        style << "padding-left:" << value << "px;padding-right:" << value << "px;";
    }
    if (GetAttribute(node, "paddingY", value)) {
        style << "padding-top:" << value << "px;padding-bottom:" << value << "px;";
    }
    if (GetAttribute(node, "margin", value)) {
        style << "margin:" << value << "px;";
    } else {
        style << "margin:0 0 16px 0;";
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style << "background:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderColor", value)) {
        style << "border:1px solid " << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderWidth", value)) {
        style << "border-width:" << value << "px;";
    }
    if (GetAttribute(node, "cornerRadius", value) || GetAttribute(node, "borderRadius", value)) {
        style << "border-radius:" << value << "px;";
    }
    if (GetAttribute(node, "opacity", value)) {
        style << "opacity:" << value << ";";
    }

    std::ostringstream html;
    html << "<div style=\"" << style.str() << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
            continue;
        }
        std::string flex;
        std::string alignSelf;
        std::string childWidth;
        std::string childHeight;
        const bool hasFlex = GetAttribute(child, "flex", flex) || GetAttribute(child, "flexGrow", flex);
        const bool hasAlign = GetAttribute(child, "alignSelf", alignSelf);
        const bool hasW = GetAttribute(child, "width", childWidth);
        const bool hasH = GetAttribute(child, "height", childHeight);
        if (hasFlex || hasAlign || hasW || hasH) {
            std::ostringstream wrapStyle;
            wrapStyle << "box-sizing:border-box;";
            if (hasFlex) {
                wrapStyle << "flex:" << flex << ";";
            }
            if (hasAlign) {
                wrapStyle << "align-self:" << alignSelf << ";";
            }
            if (hasW) {
                wrapStyle << "width:" << childWidth << ";";
            }
            if (hasH) {
                wrapStyle << "height:" << childHeight << ";";
            }
            html << "<div style=\"" << wrapStyle.str() << "\">" << ConvertNode(child) << "</div>";
        } else {
            html << ConvertNode(child);
        }
    }
    html << "</div>";
    return html.str();
}

std::string HtmlFromXml::Convert(const std::string& xml) const {
    XmlParser parser;
    const XmlNode root = parser.Parse(xml);
    return ConvertNode(root);
}

}  // namespace Robo
