#include "WebPlatform.hpp"
#include "HtmlFromXml.hpp"
#include "XmlRuntime.hpp"
#include "XmlToCpp.hpp"
#include "Robo/FontEngine.hpp"

#include <emscripten/bind.h>

#include <exception>
#include <memory>
#include <string>

namespace {

std::unique_ptr<Robo::WebPlatform> gPlatform;
std::unique_ptr<Robo::FontEngine> gFonts;
std::unique_ptr<Robo::XmlRuntime> gRuntime;

void EnsureRuntime() {
    if (!gPlatform) {
        gPlatform = std::make_unique<Robo::WebPlatform>();
    }
    if (!gFonts) {
        gFonts = std::make_unique<Robo::FontEngine>();
    }
    if (!gRuntime) {
        gRuntime = std::make_unique<Robo::XmlRuntime>(*gPlatform, *gFonts);
    }
}

std::string RenderXml(const std::string& xml, int width, int height) {
    EnsureRuntime();
    gPlatform->SetSurfaceSize(width, height);
    const bool ok = gRuntime->RenderXml(xml);
    if (!ok) {
        return gRuntime->GetLastError();
    }
    return "";
}

std::string XmlToHtml(const std::string& xml) {
    try {
        Robo::HtmlFromXml converter;
        return converter.Convert(xml);
    } catch (const std::exception& exception) {
        return std::string("<!-- error: ") + exception.what() + " -->";
    }
}

std::string XmlToCppCode(const std::string& xml) {
    try {
        Robo::XmlToCpp converter;
        return converter.Convert(xml);
    } catch (const std::exception& exception) {
        return std::string("// error: ") + exception.what();
    }
}

}  // namespace

EMSCRIPTEN_BINDINGS(RoboModule) {
    emscripten::function("RenderXml", &RenderXml);
    emscripten::function("XmlToHtml", &XmlToHtml);
    emscripten::function("XmlToCpp", &XmlToCppCode);
}
