#include "HtmlFromXml.hpp"
#include "XmlParser.hpp"

#include "Robo/Color.hpp"
#include "Robo/FontStyle.hpp"
#include "Robo/TextAlign.hpp"

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

std::string HtmlFromXml::BuildTextHtml(const XmlNode& node) const {
    std::ostringstream html;
    html << "<p style=\"" << StyleFromAttributes(node, true) << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
        } else if (child.name == "Span" || child.name == "span") {
            html << BuildSpanHtml(child);
        }
    }
    html << "</p>";
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
        return BuildTextHtml(node);
    }

    return "";
}

std::string HtmlFromXml::Convert(const std::string& xml) const {
    XmlParser parser;
    const XmlNode root = parser.Parse(xml);
    return ConvertNode(root);
}

}  // namespace Robo
