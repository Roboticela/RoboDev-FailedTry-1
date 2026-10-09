#include "WebPlatform.hpp"

#include "Robo/FontStyle.hpp"
#include "Robo/TextAlign.hpp"

#include <cstdlib>
#include <emscripten.h>
#include <sstream>

namespace Robo {

namespace {

EM_JS(void, JsClearRoot, (const char* rootId), {
    const root = document.getElementById(UTF8ToString(rootId));
    if (!root) {
        return;
    }
    root.innerHTML = "";
});

EM_JS(void, JsRemoveElement, (const char* elementId), {
    const element = document.getElementById(UTF8ToString(elementId));
    if (element && element.parentNode) {
        element.parentNode.removeChild(element);
    }
});

EM_JS(void, JsMountText, (const char* parentId, const char* elementId, const char* css, const char* html), {
    const parent = document.getElementById(UTF8ToString(parentId));
    if (!parent) {
        return;
    }
    const element = document.createElement("p");
    element.id = UTF8ToString(elementId);
    element.setAttribute("style", UTF8ToString(css));
    element.innerHTML = UTF8ToString(html);
    parent.appendChild(element);
});

EM_JS(void, JsUpdateText, (const char* elementId, const char* css, const char* html), {
    const element = document.getElementById(UTF8ToString(elementId));
    if (!element) {
        return;
    }
    element.setAttribute("style", UTF8ToString(css));
    element.innerHTML = UTF8ToString(html);
});

EM_JS(char*, JsMeasureText, (const char* content, double fontSize, const char* fontWeight), {
    const canvas = document.createElement("canvas");
    const context = canvas.getContext("2d");
    context.font = UTF8ToString(fontWeight) + " " + fontSize + "px Inter, sans-serif";
    const width = context.measureText(UTF8ToString(content)).width;
    const result = String(width);
    const length = lengthBytesUTF8(result) + 1;
    const pointer = _malloc(length);
    stringToUTF8(result, pointer, length);
    return pointer;
});

std::string EscapeHtml(const std::string& value) {
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

std::string BuildInnerHtml(const Text& text) {
    std::ostringstream html;
    const TextStyle& style = text.GetStyle();

    for (const TextRun& run : text.GetRuns()) {
        const double fontSize = run.hasFontSize ? run.fontSize : style.fontSize;
        const Color color = run.hasColor ? run.color : style.color;
        const FontWeight fontWeight = run.hasFontWeight ? run.fontWeight : style.fontWeight;
        const FontStyleKind fontStyle = run.hasFontStyle ? run.fontStyle : style.fontStyle;
        const TextDecoration decoration = run.hasTextDecoration ? run.textDecoration : style.textDecoration;
        const double letterSpacing = run.hasLetterSpacing ? run.letterSpacing : style.letterSpacing;
        const double opacity = run.hasOpacity ? run.opacity : style.opacity;

        html << "<span style=\""
             << "font-size:" << fontSize << "px;"
             << "color:" << color.ToCss() << ";"
             << "font-weight:" << FontWeightToCss(fontWeight) << ";"
             << "font-style:" << FontStyleToCss(fontStyle) << ";"
             << "text-decoration:" << TextDecorationToCss(decoration) << ";"
             << "letter-spacing:" << letterSpacing << "px;"
             << "opacity:" << opacity << ";"
             << "\">" << EscapeHtml(run.content) << "</span>";
    }

    return html.str();
}

}  // namespace

std::string WebPlatform::NextElementId() {
    std::ostringstream stream;
    stream << "robo-text-" << nextId_++;
    return stream.str();
}

std::string WebPlatform::BuildCss(const Text& text) const {
    const TextStyle& style = text.GetStyle();
    std::ostringstream css;
    css << "margin:0 0 0.75rem 0;"
        << "font-family:'Inter',sans-serif;"
        << "font-size:" << style.fontSize << "px;"
        << "color:" << style.color.ToCss() << ";"
        << "font-weight:" << FontWeightToCss(style.fontWeight) << ";"
        << "font-style:" << FontStyleToCss(style.fontStyle) << ";"
        << "text-decoration:" << TextDecorationToCss(style.textDecoration) << ";"
        << "text-align:" << TextAlignToCss(style.align) << ";"
        << "line-height:" << style.lineHeight << ";"
        << "letter-spacing:" << style.letterSpacing << "px;"
        << "opacity:" << style.opacity << ";";

    if (!style.width.empty()) {
        css << "width:" << style.width << ";";
    }
    if (!style.maxWidth.empty()) {
        css << "max-width:" << style.maxWidth << ";";
    }

    if (style.wrap) {
        css << "white-space:normal;overflow-wrap:anywhere;";
    } else {
        css << "white-space:nowrap;overflow:hidden;text-overflow:ellipsis;";
    }

    return css.str();
}

void WebPlatform::ClearRoot(const std::string& rootId) {
    JsClearRoot(rootId.c_str());
}

std::string WebPlatform::MountTextElement(const std::string& parentId, const Text& text) {
    const std::string elementId = NextElementId();
    const std::string css = BuildCss(text);
    const std::string html = BuildInnerHtml(text);
    JsMountText(parentId.c_str(), elementId.c_str(), css.c_str(), html.c_str());
    return elementId;
}

void WebPlatform::UpdateTextElement(const std::string& elementId, const Text& text) {
    const std::string css = BuildCss(text);
    const std::string html = BuildInnerHtml(text);
    JsUpdateText(elementId.c_str(), css.c_str(), html.c_str());
}

void WebPlatform::RemoveElement(const std::string& elementId) {
    JsRemoveElement(elementId.c_str());
}

std::string WebPlatform::MeasureText(const std::string& content, double fontSize, const std::string& fontWeight) {
    char* measured = JsMeasureText(content.c_str(), fontSize, fontWeight.c_str());
    std::string result = measured ? measured : "0";
    free(measured);
    return result;
}

}  // namespace Robo
