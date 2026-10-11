#include "XmlToCpp.hpp"
#include "IconPack.hpp"
#include "XmlParser.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace Robo {

std::string XmlToCpp::Indent(int level) const {
    return std::string(static_cast<std::size_t>(level) * 4, ' ');
}

std::string XmlToCpp::EscapeCpp(const std::string& value) const {
    std::string result;
    result.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\':
                result += "\\\\";
                break;
            case '"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result.push_back(ch);
                break;
        }
    }
    return result;
}

std::string XmlToCpp::FloatLiteral(const std::string& value) const {
    // A bare "16f" is not valid C++, so every numeric attribute is normalised
    // into a real float literal before it reaches the generated source.
    std::string trimmed = value;
    const std::size_t first = trimmed.find_first_not_of(" 	");
    if (first == std::string::npos) {
        return "0.0f";
    }
    trimmed.erase(0, first);
    trimmed.erase(trimmed.find_last_not_of(" 	") + 1);
    if (trimmed.find('.') == std::string::npos && trimmed.find('e') == std::string::npos &&
        trimmed.find('E') == std::string::npos) {
        trimmed += ".0";
    }
    return trimmed + "f";
}

std::string XmlToCpp::ConvertNode(const XmlNode& node, int indent) const {
    std::ostringstream code;
    if (node.isText) {
        return code.str();
    }

    if (node.name == "Robo") {
        code << Indent(indent) << "// Generated from Robo XML\n";
        code << Indent(indent) << "void BuildPage(Robo::RoboScene& scene) {\n";
        for (const XmlNode& child : node.children) {
            code << ConvertNode(child, indent + 1);
        }
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Text") {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";

        for (const auto& attribute : node.attributes) {
            if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "align") {
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "wrap") {
                const bool wrap = !(attribute.second == "false" || attribute.second == "0" || attribute.second == "no");
                code << Indent(indent + 1) << "style.wrap = " << (wrap ? "true" : "false") << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "fontStyle") {
                code << Indent(indent + 1) << "style.fontStyle = Robo::ParseFontStyle(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "textDecoration") {
                code << Indent(indent + 1) << "style.textDecoration = Robo::ParseTextDecoration(\""
                     << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "lineHeight") {
                code << Indent(indent + 1) << "style.lineHeight = " << attribute.second << ";\n";
            } else if (attribute.first == "letterSpacing") {
                code << Indent(indent + 1) << "style.letterSpacing = " << attribute.second << ";\n";
            } else if (attribute.first == "opacity") {
                code << Indent(indent + 1) << "style.opacity = " << attribute.second << ";\n";
            } else if (attribute.first == "width") {
                code << Indent(indent + 1) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
            } else if (attribute.first == "maxWidth") {
                code << Indent(indent + 1) << "style.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        }

        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";

        for (const XmlNode& child : node.children) {
            if (child.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(child.text) << "\");\n";
            } else if (child.name == "Span" || child.name == "span") {
                code << Indent(indent + 1) << "{\n";
                code << Indent(indent + 2) << "Robo::TextRun run;\n";
                std::string content;
                for (const XmlNode& spanChild : child.children) {
                    if (spanChild.isText) {
                        content += spanChild.text;
                    }
                }
                code << Indent(indent + 2) << "run.content = \"" << EscapeCpp(content) << "\";\n";
                for (const auto& attribute : child.attributes) {
                    if (attribute.first == "fontSize") {
                        code << Indent(indent + 2) << "run.fontSize = " << attribute.second << ";\n";
                        code << Indent(indent + 2) << "run.hasFontSize = true;\n";
                    } else if (attribute.first == "color") {
                        code << Indent(indent + 2) << "run.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                             << "\");\n";
                        code << Indent(indent + 2) << "run.hasColor = true;\n";
                    } else if (attribute.first == "fontWeight") {
                        code << Indent(indent + 2) << "run.fontWeight = Robo::ParseFontWeight(\""
                             << EscapeCpp(attribute.second) << "\");\n";
                        code << Indent(indent + 2) << "run.hasFontWeight = true;\n";
                    } else if (attribute.first == "fontStyle") {
                        code << Indent(indent + 2) << "run.fontStyle = Robo::ParseFontStyle(\""
                             << EscapeCpp(attribute.second) << "\");\n";
                        code << Indent(indent + 2) << "run.hasFontStyle = true;\n";
                    } else if (attribute.first == "textDecoration") {
                        code << Indent(indent + 2) << "run.textDecoration = Robo::ParseTextDecoration(\""
                             << EscapeCpp(attribute.second) << "\");\n";
                        code << Indent(indent + 2) << "run.hasTextDecoration = true;\n";
                    } else if (attribute.first == "letterSpacing") {
                        code << Indent(indent + 2) << "run.letterSpacing = " << attribute.second << ";\n";
                        code << Indent(indent + 2) << "run.hasLetterSpacing = true;\n";
                    } else if (attribute.first == "opacity") {
                        code << Indent(indent + 2) << "run.opacity = " << attribute.second << ";\n";
                        code << Indent(indent + 2) << "run.hasOpacity = true;\n";
                    }
                }
                code << Indent(indent + 2) << "text.AddRun(run);\n";
                code << Indent(indent + 1) << "}\n";
            }
        }

        code << Indent(indent + 1) << "scene.AddText(std::move(text));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    const bool isHeading =
        node.name == "Heading" || node.name == "heading" ||
        (node.name.size() == 2 && (node.name[0] == 'H' || node.name[0] == 'h') && node.name[1] >= '1' &&
         node.name[1] <= '6') ||
        (node.name.size() == 8 && (node.name.compare(0, 7, "Heading") == 0 || node.name.compare(0, 7, "heading") == 0) &&
         node.name[7] >= '1' && node.name[7] <= '6');
    const bool isParagraph =
        node.name == "Paragraph" || node.name == "paragraph" || node.name == "P" || node.name == "p";

    if (isHeading || isParagraph) {
        int level = 1;
        if (isHeading) {
            for (const auto& attribute : node.attributes) {
                if (attribute.first == "level") {
                    level = std::atoi(attribute.second.c_str());
                }
            }
            if (node.name.size() == 2 && node.name[1] >= '1' && node.name[1] <= '6') {
                level = node.name[1] - '0';
            }
            if (node.name.size() == 8 && node.name[7] >= '1' && node.name[7] <= '6') {
                level = node.name[7] - '0';
            }
        }

        code << Indent(indent) << "{\n";
        if (isHeading) {
            code << Indent(indent + 1) << "Robo::Heading text(" << level << ");\n";
        } else {
            code << Indent(indent + 1) << "Robo::Paragraph text;\n";
        }
        code << Indent(indent + 1) << "Robo::TextStyle style = text.GetStyle();\n";

        for (const auto& attribute : node.attributes) {
            if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "align") {
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "wrap") {
                const bool wrap = !(attribute.second == "false" || attribute.second == "0" || attribute.second == "no");
                code << Indent(indent + 1) << "style.wrap = " << (wrap ? "true" : "false") << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "lineHeight") {
                code << Indent(indent + 1) << "style.lineHeight = " << attribute.second << ";\n";
            } else if (attribute.first == "width") {
                code << Indent(indent + 1) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
            } else if (attribute.first == "maxWidth") {
                code << Indent(indent + 1) << "style.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";

        for (const XmlNode& child : node.children) {
            if (child.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(child.text) << "\");\n";
            } else if (child.name == "Span" || child.name == "span") {
                code << Indent(indent + 1) << "{\n";
                code << Indent(indent + 2) << "Robo::TextRun run;\n";
                std::string content;
                for (const XmlNode& spanChild : child.children) {
                    if (spanChild.isText) {
                        content += spanChild.text;
                    }
                }
                code << Indent(indent + 2) << "run.content = \"" << EscapeCpp(content) << "\";\n";
                code << Indent(indent + 2) << "text.AddRun(run);\n";
                code << Indent(indent + 1) << "}\n";
            }
        }

        code << Indent(indent + 1) << "scene.AddText(std::move(text));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Button") {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";

        std::string label;
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "title" || attribute.first == "label") {
                label = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            } else if (attribute.first == "paddingX") {
                code << Indent(indent + 1) << "style.paddingX = " << attribute.second << ";\n";
            } else if (attribute.first == "paddingY") {
                code << Indent(indent + 1) << "style.paddingY = " << attribute.second << ";\n";
            } else if (attribute.first == "align") {
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            }
        }

        if (label.empty()) {
            for (const XmlNode& child : node.children) {
                if (child.isText) {
                    label += child.text;
                }
            }
        }

        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        code << Indent(indent + 1) << "scene.AddButton(std::move(button));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Input" || node.name == "TextField" || node.name == "TextInput") {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";

        std::string value;
        std::string placeholder;
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "value" || attribute.first == "text") {
                value = attribute.second;
            } else if (attribute.first == "placeholder" || attribute.first == "hint") {
                placeholder = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            } else if (attribute.first == "width") {
                code << Indent(indent + 1) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
            } else if (attribute.first == "align") {
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            }
        }

        if (value.empty()) {
            for (const XmlNode& child : node.children) {
                if (child.isText) {
                    value += child.text;
                }
            }
        }

        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << "scene.AddInput(std::move(input));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Scrollbar" || node.name == "scrollbar") {
        code << Indent(indent) << "{\n";
        EmitScrollbar(code, node, indent + 1, "scrollbar");
        code << Indent(indent + 1) << "scene.AddScrollbar(std::move(scrollbar));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Container" || node.name == "container") {
        code << Indent(indent) << "{\n";
        EmitContainer(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Flexbox" || node.name == "flexbox" || node.name == "Flex" || node.name == "flex") {
        code << Indent(indent) << "{\n";
        EmitFlexbox(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Grid" || node.name == "grid") {
        code << Indent(indent) << "{\n";
        EmitGrid(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Position" || node.name == "position" || node.name == "Positioned" || node.name == "positioned") {
        code << Indent(indent) << "{\n";
        EmitPosition(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (TryEmitGraphic(code, node, indent + 1, "scene", false)) {
        return code.str();
    }

    if (!node.name.empty()) {
        std::cerr << "Skipping unknown XML element <" << node.name << ">\n";
    }
    return code.str();
}

void XmlToCpp::EmitScrollbar(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    code << Indent(indent) << "Robo::Scrollbar " << targetVar << ";\n";
    code << Indent(indent) << "Robo::ScrollbarStyle style;\n";

    // Presets come first so that explicit attributes emitted below win.
    const auto variantIt = node.attributes.find("variant");
    if (variantIt != node.attributes.end() &&
        (variantIt->second == "thin" || variantIt->second == "overlay")) {
        code << Indent(indent) << "style.thickness = 14.0;\n";
        code << Indent(indent) << "style.cornerRadius = 7.0;\n";
        code << Indent(indent) << "style.minThumbLength = 28.0;\n";
        code << Indent(indent) << "style.arrowSize = 16.0;\n";
        code << Indent(indent) << "style.showArrows = true;\n";
    }

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "orientation" || attribute.first == "direction") {
            code << Indent(indent) << "style.orientation = Robo::ParseScrollbarOrientation(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "thickness" || attribute.first == "size") {
            code << Indent(indent) << "style.thickness = " << attribute.second << ";\n";
        } else if (attribute.first == "length" || attribute.first == "trackLength") {
            code << Indent(indent) << "style.length = " << attribute.second << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "radius") {
            code << Indent(indent) << "style.cornerRadius = " << attribute.second << ";\n";
        } else if (attribute.first == "minThumbLength" || attribute.first == "minThumb") {
            code << Indent(indent) << "style.minThumbLength = " << attribute.second << ";\n";
        } else if (attribute.first == "trackColor" || attribute.first == "track") {
            code << Indent(indent) << "style.trackColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomTrackColor = true;\n";
        } else if (attribute.first == "thumbColor" || attribute.first == "color") {
            code << Indent(indent) << "style.thumbColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomThumbColor = true;\n";
        } else if (attribute.first == "thumbHoverColor") {
            code << Indent(indent) << "style.thumbHoverColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomThumbHoverColor = true;\n";
        } else if (attribute.first == "thumbActiveColor") {
            code << Indent(indent) << "style.thumbActiveColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomThumbActiveColor = true;\n";
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << attribute.second << ";\n";
        } else if (attribute.first == "visibility" || attribute.first == "showWhen") {
            code << Indent(indent) << "style.visibility = Robo::ParseScrollbarVisibility(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "arrows" || attribute.first == "showArrows" ||
                   attribute.first == "arrow") {
            code << Indent(indent) << "style.showArrows = Robo::ParseScrollbarFlag(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
        } else if (attribute.first == "arrowSize") {
            code << Indent(indent) << "style.arrowSize = " << attribute.second << ";\n";
        } else if (attribute.first == "arrowGap") {
            code << Indent(indent) << "style.arrowGap = " << attribute.second << ";\n";
        } else if (attribute.first == "arrowColor") {
            code << Indent(indent) << "style.arrowColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomArrowColor = true;\n";
        } else if (attribute.first == "arrowHoverColor") {
            code << Indent(indent) << "style.arrowHoverColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomArrowHoverColor = true;\n";
        } else if (attribute.first == "arrowActiveColor") {
            code << Indent(indent) << "style.arrowActiveColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.hasCustomArrowActiveColor = true;\n";
        } else if (attribute.first == "autoHide" || attribute.first == "autohide") {
            code << Indent(indent) << "style.autoHide = Robo::ParseScrollbarFlag(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
        } else if (attribute.first == "autoHideDelay" || attribute.first == "hideDelay" ||
                   attribute.first == "hideAfter") {
            code << Indent(indent) << "style.autoHideDelay = " << attribute.second << ";\n";
        } else if (attribute.first == "fadeDuration" || attribute.first == "fade") {
            code << Indent(indent) << "style.fadeDuration = " << attribute.second << ";\n";
        } else if (attribute.first == "hiddenOpacity" || attribute.first == "idleOpacity") {
            code << Indent(indent) << "style.hiddenOpacity = " << attribute.second << ";\n";
        }
    }

    code << Indent(indent) << targetVar << ".SetStyle(style);\n";
    code << Indent(indent) << targetVar << ".ApplyVariantDefaults();\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "value") {
            code << Indent(indent) << targetVar << ".SetValue(" << attribute.second << ");\n";
        } else if (attribute.first == "contentSize" || attribute.first == "content") {
            code << Indent(indent) << targetVar << ".SetContentSize(" << attribute.second << ");\n";
        } else if (attribute.first == "viewportSize" || attribute.first == "viewport" ||
                   attribute.first == "pageSize") {
            code << Indent(indent) << targetVar << ".SetViewportSize(" << attribute.second << ");\n";
        } else if (attribute.first == "thumbRatio" || attribute.first == "thumbSize") {
            code << Indent(indent) << targetVar << ".SetThumbRatio(" << attribute.second << ");\n";
        }
    }
}

void XmlToCpp::EmitContainer(std::ostringstream& code,
                             const XmlNode& node,
                             int indent,
                             const std::string& targetVar) const {
    const std::string containerVar = (targetVar == "scene") ? "container" : targetVar;
    code << Indent(indent) << "Robo::Container " << containerVar << ";\n";
    code << Indent(indent) << "Robo::ContainerStyle style;\n";
    // A container hides its bar unless the content overflows, like an overlay.
    code << Indent(indent) << "style.scrollbarStyle.visibility = Robo::ScrollbarVisibility::Auto;\n";

    // Presets come first so the explicit attributes emitted below win.
    const auto variantIt = node.attributes.find("variant");
    if (variantIt != node.attributes.end() &&
        (variantIt->second == "thin" || variantIt->second == "overlay")) {
        code << Indent(indent) << "style.scrollbarStyle.thickness = 14.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.cornerRadius = 7.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.minThumbLength = 28.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.arrowSize = 16.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.showArrows = true;\n";
    }

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "layout") {
            code << Indent(indent) << "style.layout = Robo::ParseContainerLayout(\"" << EscapeCpp(attribute.second)
                 << "\");\n";
        } else if (attribute.first == "orientation" || attribute.first == "direction" || attribute.first == "axis" ||
            attribute.first == "scrollAxis") {
            code << Indent(indent) << "style.orientation = Robo::ParseContainerOrientation(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            code << Indent(indent) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent) << "style.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent) << "style.minHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent) << "style.maxHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "padding") {
            code << Indent(indent) << "style.paddingTop = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingLeft = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingRight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "paddingX") {
            code << Indent(indent) << "style.paddingLeft = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingRight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "paddingY") {
            code << Indent(indent) << "style.paddingTop = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "gap" || attribute.first == "itemGap") {
            code << Indent(indent) << "style.gap = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << Indent(indent) << "style.backgroundColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderColor") {
            code << Indent(indent) << "style.borderColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << Indent(indent) << "style.cornerRadius = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "clip" || attribute.first == "clipToBounds") {
            code << Indent(indent) << "style.clipToBounds = Robo::ParseBool(\"" << EscapeCpp(attribute.second)
                 << "\", true);\n";
        } else if (attribute.first == "showScrollbar" || attribute.first == "scrollbar") {
            code << Indent(indent) << "style.showScrollbar = Robo::ParseBool(\"" << EscapeCpp(attribute.second)
                 << "\", true);\n";
        } else if (attribute.first == "scrollbarGap") {
            code << Indent(indent) << "style.scrollbarGap = " << attribute.second << ";\n";
        } else if (attribute.first == "contentSize" || attribute.first == "contentExtent" ||
                   attribute.first == "scrollSize") {
            code << Indent(indent) << "style.contentExtent = " << attribute.second << ";\n";
        } else if (attribute.first == "viewportSize" || attribute.first == "viewportExtent") {
            code << Indent(indent) << "style.viewportExtent = " << attribute.second << ";\n";
        } else if (attribute.first == "variant") {
            // Handled by the preset above.
        } else if (attribute.first == "thickness" || attribute.first == "size") {
            code << Indent(indent) << "style.scrollbarStyle.thickness = " << attribute.second << ";\n";
        } else if (attribute.first == "barRadius") {
            code << Indent(indent) << "style.scrollbarStyle.cornerRadius = " << attribute.second << ";\n";
        } else if (attribute.first == "minThumbLength") {
            code << Indent(indent) << "style.scrollbarStyle.minThumbLength = " << attribute.second << ";\n";
        } else if (attribute.first == "trackColor") {
            code << Indent(indent) << "style.scrollbarStyle.trackColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.scrollbarStyle.hasCustomTrackColor = true;\n";
        } else if (attribute.first == "thumbColor") {
            code << Indent(indent) << "style.scrollbarStyle.thumbColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.scrollbarStyle.hasCustomThumbColor = true;\n";
        } else if (attribute.first == "thumbHoverColor") {
            code << Indent(indent) << "style.scrollbarStyle.thumbHoverColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.scrollbarStyle.hasCustomThumbHoverColor = true;\n";
        } else if (attribute.first == "thumbActiveColor") {
            code << Indent(indent) << "style.scrollbarStyle.thumbActiveColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.scrollbarStyle.hasCustomThumbActiveColor = true;\n";
        } else if (attribute.first == "arrows" || attribute.first == "showArrows" || attribute.first == "arrow") {
            code << Indent(indent) << "style.scrollbarStyle.showArrows = Robo::ParseBool(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
        } else if (attribute.first == "arrowColor") {
            code << Indent(indent) << "style.scrollbarStyle.arrowColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
            code << Indent(indent) << "style.scrollbarStyle.hasCustomArrowColor = true;\n";
        } else if (attribute.first == "autoHide" || attribute.first == "autohide") {
            code << Indent(indent) << "style.scrollbarStyle.autoHide = Robo::ParseBool(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
        } else if (attribute.first == "autoHideDelay") {
            code << Indent(indent) << "style.scrollbarStyle.autoHideDelay = " << attribute.second << ";\n";
        } else if (attribute.first == "fadeDuration") {
            code << Indent(indent) << "style.scrollbarStyle.fadeDuration = " << attribute.second << ";\n";
        } else if (attribute.first == "hiddenOpacity") {
            code << Indent(indent) << "style.scrollbarStyle.hiddenOpacity = " << attribute.second << ";\n";
        } else if (attribute.first == "visibility") {
            code << Indent(indent) << "style.scrollbarStyle.visibility = Robo::ParseScrollbarVisibility(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << containerVar << ".SetStyle(style);\n";
    code << Indent(indent) << containerVar << ".RefreshVariantDefaults();\n";

    std::string layout = "stack";
    const auto layoutIt = node.attributes.find("layout");
    if (layoutIt != node.attributes.end()) {
        layout = layoutIt->second;
    }

    if (layout == "flex" || layout == "flexbox") {
        const std::string flexName = containerVar + "Flex";
        EmitFlexbox(code, node, indent, flexName);
        code << Indent(indent) << flexName << ".GetStyle().width = \"auto\";\n";
        code << Indent(indent) << flexName << ".GetStyle().height = \"auto\";\n";
        code << Indent(indent) << flexName << ".GetStyle().paddingTop = 0.0f;\n";
        code << Indent(indent) << flexName << ".GetStyle().paddingRight = 0.0f;\n";
        code << Indent(indent) << flexName << ".GetStyle().paddingBottom = 0.0f;\n";
        code << Indent(indent) << flexName << ".GetStyle().paddingLeft = 0.0f;\n";
        code << Indent(indent) << flexName << ".GetStyle().backgroundColor = Robo::Color::FromRgba(0, 0, 0, 0);\n";
        code << Indent(indent) << flexName << ".GetStyle().borderWidth = 0.0f;\n";
        code << Indent(indent) << containerVar << ".SetFlexbox(std::move(" << flexName << "));\n";
    } else if (layout == "grid") {
        const std::string gridName = containerVar + "Grid";
        EmitGrid(code, node, indent, gridName);
        code << Indent(indent) << gridName << ".GetStyle().width = \"auto\";\n";
        code << Indent(indent) << gridName << ".GetStyle().height = \"auto\";\n";
        code << Indent(indent) << gridName << ".GetStyle().paddingTop = 0.0f;\n";
        code << Indent(indent) << gridName << ".GetStyle().paddingRight = 0.0f;\n";
        code << Indent(indent) << gridName << ".GetStyle().paddingBottom = 0.0f;\n";
        code << Indent(indent) << gridName << ".GetStyle().paddingLeft = 0.0f;\n";
        code << Indent(indent) << gridName << ".GetStyle().backgroundColor = Robo::Color::FromRgba(0, 0, 0, 0);\n";
        code << Indent(indent) << gridName << ".GetStyle().borderWidth = 0.0f;\n";
        code << Indent(indent) << containerVar << ".SetGrid(std::move(" << gridName << "));\n";
    } else {
        for (const XmlNode& child : node.children) {
            if (!child.isText) {
                EmitPositionChild(code, child, indent, containerVar + ".GetContent()");
            }
        }
    }

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "value" || attribute.first == "scroll" || attribute.first == "scrollValue") {
            code << Indent(indent) << containerVar << ".SetScrollValue(" << attribute.second << ");\n";
        }
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddContainer(std::move(" << containerVar << "));\n";
    }
}

void XmlToCpp::EmitFlexbox(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string flexVar = (targetVar == "scene") ? "flexbox" : targetVar;
    code << Indent(indent) << "Robo::Flexbox " << flexVar << ";\n";
    code << Indent(indent) << "Robo::FlexboxStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "direction" || attribute.first == "flexDirection") {
            code << Indent(indent) << "style.direction = Robo::ParseFlexDirection(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "justify" || attribute.first == "justifyContent") {
            code << Indent(indent) << "style.justify = Robo::ParseFlexJustify(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "align" || attribute.first == "alignItems") {
            code << Indent(indent) << "style.align = Robo::ParseFlexAlign(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "wrap" || attribute.first == "flexWrap") {
            code << Indent(indent) << "style.wrap = Robo::ParseFlexWrap(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "gap") {
            code << Indent(indent) << "style.gap = " << attribute.second << ";\n";
        } else if (attribute.first == "gapX" || attribute.first == "columnGap") {
            code << Indent(indent) << "style.columnGap = " << attribute.second << ";\n";
        } else if (attribute.first == "gapY" || attribute.first == "rowGap") {
            code << Indent(indent) << "style.rowGap = " << attribute.second << ";\n";
        } else if (attribute.first == "width") {
            code << Indent(indent) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent) << "style.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "padding") {
            code << Indent(indent) << "style.paddingTop = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingLeft = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingRight = " << attribute.second << ";\n";
        } else if (attribute.first == "paddingX") {
            code << Indent(indent) << "style.paddingLeft = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingRight = " << attribute.second << ";\n";
        } else if (attribute.first == "paddingY") {
            code << Indent(indent) << "style.paddingTop = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << attribute.second << ";\n";
        } else if (attribute.first == "margin") {
            code << Indent(indent) << "style.marginTop = " << attribute.second << ";\n";
            code << Indent(indent) << "style.marginBottom = " << attribute.second << ";\n";
            code << Indent(indent) << "style.marginLeft = " << attribute.second << ";\n";
            code << Indent(indent) << "style.marginRight = " << attribute.second << ";\n";
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << Indent(indent) << "style.backgroundColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderColor") {
            code << Indent(indent) << "style.borderColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << Indent(indent) << "style.cornerRadius = " << attribute.second << ";\n";
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << attribute.second << ";\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << flexVar << ".SetStyle(style);\n";

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
                code << Indent(indent) << "{\n";
                code << Indent(indent + 1) << "Robo::Text rawText;\n";
                code << Indent(indent + 1) << "rawText.AddPlain(\"" << EscapeCpp(content) << "\");\n";
                code << Indent(indent + 1) << flexVar << ".AddText(std::move(rawText));\n";
                code << Indent(indent) << "}\n";
            }
            continue;
        }

        EmitFlexChild(code, child, indent, flexVar);
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddFlexbox(std::move(" << flexVar << "));\n";
    }
}

void XmlToCpp::EmitFlexChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::FlexItemProps props;\n";
    for (const auto& attribute : child.attributes) {
        if (attribute.first == "flex" || attribute.first == "flexGrow") {
            code << Indent(indent + 1) << "props.flexGrow = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "flexShrink") {
            code << Indent(indent + 1) << "props.flexShrink = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "flexBasis") {
            if (!attribute.second.empty() && attribute.second.back() == '%') {
                code << Indent(indent + 1) << "props.flexBasisString = \"" << EscapeCpp(attribute.second) << "\";\n";
            } else {
                code << Indent(indent + 1) << "props.flexBasis = " << FloatLiteral(attribute.second) << ";\n";
            }
        } else if (attribute.first == "alignSelf") {
            code << Indent(indent + 1) << "props.alignSelf = Robo::ParseFlexAlign(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            code << Indent(indent + 1) << "props.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent + 1) << "props.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent + 1) << "props.minWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent + 1) << "props.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent + 1) << "props.minHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent + 1) << "props.maxHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "margin") {
            code << Indent(indent + 1) << "props.marginTop = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginBottom = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginLeft = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginRight = " << attribute.second << ";\n";
        }
    }

    if (child.name == "Button") {
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";
        std::string label;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "title" || attribute.first == "label") {
                label = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            } else if (attribute.first == "paddingX") {
                code << Indent(indent + 1) << "style.paddingX = " << attribute.second << ";\n";
            } else if (attribute.first == "paddingY") {
                code << Indent(indent + 1) << "style.paddingY = " << attribute.second << ";\n";
            }
        }
        if (label.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    label += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddButton(std::move(button), props);\n";
    } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";
        std::string placeholder;
        std::string value;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "placeholder" || attribute.first == "hint") {
                placeholder = attribute.second;
            } else if (attribute.first == "value" || attribute.first == "text") {
                value = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            }
        }
        if (value.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    value += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddInput(std::move(input), props);\n";
    } else if (child.name == "Scrollbar" || child.name == "scrollbar") {
        EmitScrollbar(code, child, indent + 1, "scrollbar");
        code << Indent(indent + 1) << parentVar << ".AddScrollbar(std::move(scrollbar), props);\n";
    } else if (child.name == "Container" || child.name == "container") {
        const std::string nestedContainer = "nestedContainer" + std::to_string(indent + 1);
        EmitContainer(code, child, indent + 1, nestedContainer);
        code << Indent(indent + 1) << parentVar << ".AddContainer(std::move(" << nestedContainer
             << "), props);\n";
    } else if (child.name == "Text") {
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(text), props);\n";
    } else if (child.name == "Heading" || child.name == "heading" ||
               (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' && child.name[1] <= '6') ||
               (child.name.size() == 8 && (child.name.compare(0, 7, "Heading") == 0 || child.name.compare(0, 7, "heading") == 0) && child.name[7] >= '1' && child.name[7] <= '6')) {
        int level = 1;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "level") {
                level = std::atoi(attribute.second.c_str());
            }
        }
        if (child.name.size() == 2 && child.name[1] >= '1' && child.name[1] <= '6') {
            level = child.name[1] - '0';
        }
        if (child.name.size() == 8 && child.name[7] >= '1' && child.name[7] <= '6') {
            level = child.name[7] - '0';
        }
        code << Indent(indent + 1) << "Robo::Heading heading(" << level << ");\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = heading.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "heading.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "heading.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(heading), props);\n";
    } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
        code << Indent(indent + 1) << "Robo::Paragraph paragraph;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = paragraph.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "paragraph.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "paragraph.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(paragraph), props);\n";
    } else if (child.name == "Flexbox" || child.name == "flexbox" || child.name == "Flex" || child.name == "flex") {
        const std::string childFlexVar = "nestedFlex" + std::to_string(indent + 1);
        EmitFlexbox(code, child, indent + 1, childFlexVar);
        code << Indent(indent + 1) << parentVar << ".AddFlexbox(std::move(" << childFlexVar << "), props);\n";
    } else if (child.name == "Grid" || child.name == "grid") {
        const std::string childGridVar = "nestedGrid" + std::to_string(indent + 1);
        EmitGrid(code, child, indent + 1, childGridVar);
        code << Indent(indent + 1) << parentVar << ".AddGrid(std::move(childGridVar), props);\n";
    } else if (!TryEmitGraphic(code, child, indent + 1, parentVar, true)) {
        if (!child.name.empty()) {
            std::cerr << "Skipping unknown XML element <" << child.name << ">\n";
        }
    }

    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitGrid(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string gridVar = (targetVar == "scene") ? "grid" : targetVar;
    code << Indent(indent) << "Robo::Grid " << gridVar << ";\n";
    code << Indent(indent) << "Robo::GridStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "columns" || attribute.first == "templateColumns" || attribute.first == "gridTemplateColumns") {
            code << Indent(indent) << "style.columnsString = \"" << EscapeCpp(attribute.second) << "\";\n";
            code << Indent(indent) << "style.templateColumns = Robo::ParseGridTracks(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "rows" || attribute.first == "templateRows" || attribute.first == "gridTemplateRows") {
            code << Indent(indent) << "style.rowsString = \"" << EscapeCpp(attribute.second) << "\";\n";
            code << Indent(indent) << "style.templateRows = Robo::ParseGridTracks(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "columnCount") {
            code << Indent(indent) << "style.columnCount = " << attribute.second << ";\n";
        } else if (attribute.first == "rowCount") {
            code << Indent(indent) << "style.rowCount = " << attribute.second << ";\n";
        } else if (attribute.first == "gap") {
            code << Indent(indent) << "style.gap = " << attribute.second << ";\n";
        } else if (attribute.first == "gapX" || attribute.first == "columnGap") {
            code << Indent(indent) << "style.columnGap = " << attribute.second << ";\n";
        } else if (attribute.first == "gapY" || attribute.first == "rowGap") {
            code << Indent(indent) << "style.rowGap = " << attribute.second << ";\n";
        } else if (attribute.first == "autoFlow") {
            code << Indent(indent) << "style.autoFlow = Robo::ParseGridAutoFlow(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "justifyItems" || attribute.first == "justify") {
            code << Indent(indent) << "style.justifyItems = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "alignItems" || attribute.first == "align") {
            code << Indent(indent) << "style.alignItems = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "justifyContent") {
            code << Indent(indent) << "style.justifyContent = Robo::ParseGridJustifyContent(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            code << Indent(indent) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent) << "style.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent) << "style.minHeight = " << attribute.second << ";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent) << "style.maxHeight = " << attribute.second << ";\n";
        } else if (attribute.first == "padding") {
            code << Indent(indent) << "style.paddingTop = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingLeft = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingRight = " << attribute.second << ";\n";
        } else if (attribute.first == "paddingX") {
            code << Indent(indent) << "style.paddingLeft = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingRight = " << attribute.second << ";\n";
        } else if (attribute.first == "paddingY") {
            code << Indent(indent) << "style.paddingTop = " << attribute.second << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << attribute.second << ";\n";
        } else if (attribute.first == "margin") {
            code << Indent(indent) << "style.marginTop = " << attribute.second << ";\n";
            code << Indent(indent) << "style.marginBottom = " << attribute.second << ";\n";
            code << Indent(indent) << "style.marginLeft = " << attribute.second << ";\n";
            code << Indent(indent) << "style.marginRight = " << attribute.second << ";\n";
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << Indent(indent) << "style.backgroundColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderColor") {
            code << Indent(indent) << "style.borderColor = Robo::Color::FromHex(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << Indent(indent) << "style.cornerRadius = " << attribute.second << ";\n";
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << attribute.second << ";\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << gridVar << ".SetStyle(style);\n";

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
                code << Indent(indent) << "{\n";
                code << Indent(indent + 1) << "Robo::Text rawText;\n";
                code << Indent(indent + 1) << "rawText.AddPlain(\"" << EscapeCpp(content) << "\");\n";
                code << Indent(indent + 1) << gridVar << ".AddText(std::move(rawText));\n";
                code << Indent(indent) << "}\n";
            }
            continue;
        }

        EmitGridChild(code, child, indent, gridVar);
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddGrid(std::move(" << gridVar << "));\n";
    }
}

void XmlToCpp::EmitGridChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::GridItemProps props;\n";
    for (const auto& attribute : child.attributes) {
        if (attribute.first == "column" || attribute.first == "gridColumn" || attribute.first == "col") {
            code << Indent(indent + 1) << "props.column = " << attribute.second << ";\n";
        } else if (attribute.first == "row" || attribute.first == "gridRow") {
            code << Indent(indent + 1) << "props.row = " << attribute.second << ";\n";
        } else if (attribute.first == "columnSpan" || attribute.first == "colSpan") {
            code << Indent(indent + 1) << "props.columnSpan = " << attribute.second << ";\n";
        } else if (attribute.first == "rowSpan") {
            code << Indent(indent + 1) << "props.rowSpan = " << attribute.second << ";\n";
        } else if (attribute.first == "justifySelf") {
            code << Indent(indent + 1) << "props.justifySelf = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "alignSelf") {
            code << Indent(indent + 1) << "props.alignSelf = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            code << Indent(indent + 1) << "props.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent + 1) << "props.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent + 1) << "props.minWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent + 1) << "props.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent + 1) << "props.minHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent + 1) << "props.maxHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "margin") {
            code << Indent(indent + 1) << "props.marginTop = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginBottom = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginLeft = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginRight = " << attribute.second << ";\n";
        }
    }

    if (child.name == "Button") {
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";
        std::string label;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "title" || attribute.first == "label") {
                label = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            } else if (attribute.first == "paddingX") {
                code << Indent(indent + 1) << "style.paddingX = " << attribute.second << ";\n";
            } else if (attribute.first == "paddingY") {
                code << Indent(indent + 1) << "style.paddingY = " << attribute.second << ";\n";
            }
        }
        if (label.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    label += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddButton(std::move(button), props);\n";
    } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";
        std::string placeholder;
        std::string value;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "placeholder" || attribute.first == "hint") {
                placeholder = attribute.second;
            } else if (attribute.first == "value" || attribute.first == "text") {
                value = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            }
        }
        if (value.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    value += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddInput(std::move(input), props);\n";
    } else if (child.name == "Scrollbar" || child.name == "scrollbar") {
        EmitScrollbar(code, child, indent + 1, "scrollbar");
        code << Indent(indent + 1) << parentVar << ".AddScrollbar(std::move(scrollbar), props);\n";
    } else if (child.name == "Container" || child.name == "container") {
        const std::string nestedContainer = "nestedContainer" + std::to_string(indent + 1);
        EmitContainer(code, child, indent + 1, nestedContainer);
        code << Indent(indent + 1) << parentVar << ".AddContainer(std::move(" << nestedContainer
             << "), props);\n";
    } else if (child.name == "Text") {
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontWeight") {
                code << Indent(indent + 1) << "style.fontWeight = Robo::ParseFontWeight(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(text), props);\n";
    } else if (child.name == "Heading" || child.name == "heading" ||
               (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' && child.name[1] <= '6') ||
               (child.name.size() == 8 && (child.name.compare(0, 7, "Heading") == 0 || child.name.compare(0, 7, "heading") == 0) && child.name[7] >= '1' && child.name[7] <= '6')) {
        int level = 1;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "level") {
                level = std::atoi(attribute.second.c_str());
            }
        }
        if (child.name.size() == 2 && child.name[1] >= '1' && child.name[1] <= '6') {
            level = child.name[1] - '0';
        }
        if (child.name.size() == 8 && child.name[7] >= '1' && child.name[7] <= '6') {
            level = child.name[7] - '0';
        }
        code << Indent(indent + 1) << "Robo::Heading heading(" << level << ");\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = heading.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "heading.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "heading.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(heading), props);\n";
    } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
        code << Indent(indent + 1) << "Robo::Paragraph paragraph;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = paragraph.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "paragraph.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "paragraph.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(paragraph), props);\n";
    } else if (child.name == "Flexbox" || child.name == "flexbox" || child.name == "Flex" || child.name == "flex") {
        const std::string childFlexVar = "nestedFlex" + std::to_string(indent + 1);
        EmitFlexbox(code, child, indent + 1, childFlexVar);
        code << Indent(indent + 1) << parentVar << ".AddFlexbox(std::move(childFlexVar), props);\n";
    } else if (child.name == "Grid" || child.name == "grid") {
        const std::string childGridVar = "nestedGrid" + std::to_string(indent + 1);
        EmitGrid(code, child, indent + 1, childGridVar);
        code << Indent(indent + 1) << parentVar << ".AddGrid(std::move(childGridVar), props);\n";
    } else if (!TryEmitGraphic(code, child, indent + 1, parentVar, true)) {
        if (!child.name.empty()) {
            std::cerr << "Skipping unknown XML element <" << child.name << ">\n";
        }
    }

    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitPosition(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string posVar = (targetVar == "scene") ? "position" : targetVar;
    code << Indent(indent) << "Robo::Position " << posVar << ";\n";
    code << Indent(indent) << "Robo::PositionStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "mode" || attribute.first == "position") {
            code << Indent(indent) << "style.mode = Robo::ParsePositionMode(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "anchor") {
            code << Indent(indent) << "style.anchor = Robo::ParsePositionAnchor(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "left" || attribute.first == "x") {
            code << Indent(indent) << "style.hasLeft = true;\n";
            code << Indent(indent) << "style.left = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "top" || attribute.first == "y") {
            code << Indent(indent) << "style.hasTop = true;\n";
            code << Indent(indent) << "style.top = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "right") {
            code << Indent(indent) << "style.hasRight = true;\n";
            code << Indent(indent) << "style.right = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "bottom") {
            code << Indent(indent) << "style.hasBottom = true;\n";
            code << Indent(indent) << "style.bottom = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "width") {
            code << Indent(indent) << "style.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent) << "style.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent) << "style.minHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent) << "style.maxHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "zIndex") {
            code << Indent(indent) << "style.zIndex = " << attribute.second << ";\n";
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << Indent(indent) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderColor") {
            code << Indent(indent) << "style.borderColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << Indent(indent) << "style.cornerRadius = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "padding") {
            code << Indent(indent) << "style.paddingTop = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingBottom = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingLeft = " << FloatLiteral(attribute.second) << ";\n";
            code << Indent(indent) << "style.paddingRight = " << FloatLiteral(attribute.second) << ";\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << posVar << ".SetStyle(style);\n";

    for (const XmlNode& child : node.children) {
        if (!child.isText) {
            EmitPositionChild(code, child, indent, posVar);
        }
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddPosition(std::move(" << posVar << "));\n";
    }
}

void XmlToCpp::EmitPositionChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::PositionItemProps props;\n";

    for (const auto& attribute : child.attributes) {
        if (attribute.first == "left" || attribute.first == "x") {
            code << Indent(indent + 1) << "props.hasLeft = true;\n";
            code << Indent(indent + 1) << "props.left = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "top" || attribute.first == "y") {
            code << Indent(indent + 1) << "props.hasTop = true;\n";
            code << Indent(indent + 1) << "props.top = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "right") {
            code << Indent(indent + 1) << "props.hasRight = true;\n";
            code << Indent(indent + 1) << "props.right = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "bottom") {
            code << Indent(indent + 1) << "props.hasBottom = true;\n";
            code << Indent(indent + 1) << "props.bottom = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "zIndex") {
            code << Indent(indent + 1) << "props.zIndex = " << attribute.second << ";\n";
        } else if (attribute.first == "width") {
            code << Indent(indent + 1) << "props.width = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "height") {
            code << Indent(indent + 1) << "props.height = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minWidth") {
            code << Indent(indent + 1) << "props.minWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent + 1) << "props.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent + 1) << "props.minHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent + 1) << "props.maxHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "mode" || attribute.first == "position") {
            code << Indent(indent + 1) << "props.mode = Robo::ParsePositionMode(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "anchor") {
            code << Indent(indent + 1) << "props.anchor = Robo::ParsePositionAnchor(\"" << EscapeCpp(attribute.second) << "\");\n";
        }
    }

    if (child.name == "Button") {
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";
        std::string label;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "variant") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.textColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << Indent(indent + 1) << "style.backgroundColor = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << Indent(indent + 1) << "style.cornerRadius = " << attribute.second << ";\n";
            } else if (attribute.first == "label") {
                label = attribute.second;
            }
        }
        if (label.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    label += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddButton(std::move(button), props);\n";
    } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";
        std::string placeholder;
        std::string value;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "variant") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "placeholder") {
                placeholder = attribute.second;
            } else if (attribute.first == "value") {
                value = attribute.second;
            }
        }
        if (value.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    value += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddInput(std::move(input), props);\n";
    } else if (child.name == "Scrollbar" || child.name == "scrollbar") {
        EmitScrollbar(code, child, indent + 1, "scrollbar");
        code << Indent(indent + 1) << parentVar << ".AddScrollbar(std::move(scrollbar), props);\n";
    } else if (child.name == "Container" || child.name == "container") {
        const std::string nestedContainer = "nestedContainer" + std::to_string(indent + 1);
        EmitContainer(code, child, indent + 1, nestedContainer);
        code << Indent(indent + 1) << parentVar << ".AddContainer(std::move(" << nestedContainer
             << "), props);\n";
    } else if (child.name == "Text") {
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "fontSize") {
                code << Indent(indent + 1) << "style.fontSize = " << attribute.second << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent + 1) << "style.color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(text), props);\n";
    } else if (child.name == "Heading" || child.name == "heading" ||
               (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' && child.name[1] <= '6')) {
        int level = (child.name.size() == 2) ? (child.name[1] - '0') : 1;
        code << Indent(indent + 1) << "Robo::Heading heading(" << level << ");\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = heading.GetStyle();\n";
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "heading.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "heading.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(heading), props);\n";
    } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
        code << Indent(indent + 1) << "Robo::Paragraph paragraph;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = paragraph.GetStyle();\n";
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "paragraph.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "paragraph.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(paragraph), props);\n";
    } else if (child.name == "Flexbox" || child.name == "flexbox") {
        const std::string childFlexVar = "nestedFlex" + std::to_string(indent + 1);
        EmitFlexbox(code, child, indent + 1, childFlexVar);
        code << Indent(indent + 1) << parentVar << ".AddFlexbox(std::move(" << childFlexVar << "), props);\n";
    } else if (child.name == "Grid" || child.name == "grid") {
        const std::string childGridVar = "nestedGrid" + std::to_string(indent + 1);
        EmitGrid(code, child, indent + 1, childGridVar);
        code << Indent(indent + 1) << parentVar << ".AddGrid(std::move(" << childGridVar << "), props);\n";
    } else if (child.name == "Position" || child.name == "position") {
        const std::string childPosVar = "nestedPos" + std::to_string(indent + 1);
        EmitPosition(code, child, indent + 1, childPosVar);
        code << Indent(indent + 1) << parentVar << ".AddPosition(std::move(" << childPosVar << "), props);\n";
    } else if (!TryEmitGraphic(code, child, indent + 1, parentVar, true)) {
        if (!child.name.empty()) {
            std::cerr << "Skipping unknown XML element <" << child.name << ">\n";
        }
    }

    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitSelectable(std::ostringstream& code,
                              int indent,
                              const std::string& styleName,
                              const XmlNode& node) const {
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "selectable" || attribute.first == "select") {
            code << Indent(indent) << styleName << ".selectable = Robo::ParseSelectable(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        }
    }
}

bool XmlToCpp::TryEmitGraphic(std::ostringstream& code,
                               const XmlNode& node,
                               int indent,
                               const std::string& parent,
                               bool withProps) const {
    const bool isSvg = node.name == "svg" || node.name == "Svg" || node.name == "SVG";
    const auto icon = icons_.find(node.name);
    const bool isIcon = icon != icons_.end();
    if (!isSvg && !isIcon) {
        return false;
    }

    const std::string props = withProps ? ", props" : "";
    if (!withProps) {
        code << Indent(indent - 1) << "{\n";
    }

    auto emitBox = [&](const std::string& styleName) {
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "width") {
                if (attribute.second.find('%') != std::string::npos) {
                    code << Indent(indent) << styleName << ".widthSpec = \"" << EscapeCpp(attribute.second) << "\";\n";
                } else {
                    code << Indent(indent) << styleName << ".width = " << FloatLiteral(attribute.second) << ";\n";
                }
            } else if (attribute.first == "height") {
                if (attribute.second.find('%') != std::string::npos) {
                    code << Indent(indent) << styleName << ".heightSpec = \"" << EscapeCpp(attribute.second) << "\";\n";
                } else {
                    code << Indent(indent) << styleName << ".height = " << FloatLiteral(attribute.second) << ";\n";
                }
            } else if (attribute.first == "size") {
                code << Indent(indent) << styleName << ".width = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent) << styleName << ".height = " << FloatLiteral(attribute.second) << ";\n";
            } else if (attribute.first == "color") {
                code << Indent(indent) << styleName << ".color = Robo::Color::FromHex(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "opacity") {
                code << Indent(indent) << styleName << ".opacity = " << FloatLiteral(attribute.second) << ";\n";
            } else if (attribute.first == "align") {
                code << Indent(indent) << styleName << ".align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
    };

    if (isIcon) {
        code << Indent(indent) << "Robo::Icon icon;\n";
        code << Indent(indent) << "icon.SetViewBox(" << FloatLiteral(std::to_string(icon->second.viewX)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewY)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewWidth)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewHeight)) << ");\n";
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::SvgStyle packedStyle;\n";
        emitBox("packedStyle");
        code << Indent(indent) << "if (packedStyle.width > 0.0f) icon.SetWidth(packedStyle.width);\n";
        code << Indent(indent) << "if (packedStyle.height > 0.0f) icon.SetHeight(packedStyle.height);\n";
        code << Indent(indent) << "if (!packedStyle.widthSpec.empty()) icon.SetWidthSpec(packedStyle.widthSpec);\n";
        code << Indent(indent) << "if (!packedStyle.heightSpec.empty()) icon.SetHeightSpec(packedStyle.heightSpec);\n";
        code << Indent(indent) << "icon.SetColor(packedStyle.color);\n";
        code << Indent(indent) << "icon.SetOpacity(packedStyle.opacity);\n";
        code << Indent(indent) << "icon.SetAlign(packedStyle.align);\n";
        code << Indent(indent) << "}\n";
        code << EmitCompiledShapes(icon->second.shapes, indent, "icon", true);
        code << Indent(indent) << parent << ".AddSvg(icon.Bake()" << props << ");\n";
    } else {
        const CompiledSvg graphic = CompileSvgElement(node, false);
        code << Indent(indent) << "Robo::Svg svg;\n";
        code << Indent(indent) << "Robo::SvgStyle style;\n";
        code << Indent(indent) << "style.viewX = " << FloatLiteral(std::to_string(graphic.viewX)) << ";\n";
        code << Indent(indent) << "style.viewY = " << FloatLiteral(std::to_string(graphic.viewY)) << ";\n";
        code << Indent(indent) << "style.viewWidth = " << FloatLiteral(std::to_string(graphic.viewWidth)) << ";\n";
        code << Indent(indent) << "style.viewHeight = " << FloatLiteral(std::to_string(graphic.viewHeight)) << ";\n";
        emitBox("style");
        EmitSelectable(code, indent, "style", node);
        code << Indent(indent) << "svg.SetStyle(style);\n";
        code << EmitCompiledShapes(graphic.shapes, indent, "svg", false);
        code << Indent(indent) << parent << ".AddSvg(std::move(svg)" << props << ");\n";
    }

    if (!withProps) {
        code << Indent(indent - 1) << "}\n";
    }
    return true;
}

std::string XmlToCpp::Convert(const std::string& xml, const std::string& assetDirectory) const {
    icons_ = LoadIconPacks(assetDirectory);
    XmlParser parser;
    const XmlNode root = parser.Parse(xml);
    std::ostringstream code;
    code << "#include \"Robo/Text.hpp\"\n";
    code << "#include \"Robo/Svg.hpp\"\n";
    code << "#include \"Robo/Icon.hpp\"\n";
    code << "#include \"Robo/Heading.hpp\"\n";
    code << "#include \"Robo/Paragraph.hpp\"\n";
    code << "#include \"Robo/Button.hpp\"\n";
    code << "#include \"Robo/Input.hpp\"\n";
    code << "#include \"Robo/Scrollbar.hpp\"\n";
    code << "#include \"Robo/Container.hpp\"\n";
    code << "#include \"Robo/Flexbox.hpp\"\n";
    code << "#include \"Robo/Grid.hpp\"\n";
    code << "#include \"Robo/Position.hpp\"\n";
    code << "#include \"Robo/RoboScene.hpp\"\n";
    code << "#include \"Robo/Color.hpp\"\n";
    code << "#include \"Robo/FontStyle.hpp\"\n";
    code << "#include \"Robo/TextAlign.hpp\"\n";
    code << "#include \"Robo/Selectable.hpp\"\n\n";
    code << ConvertNode(root, 0);
    return code.str();
}

}  // namespace Robo
