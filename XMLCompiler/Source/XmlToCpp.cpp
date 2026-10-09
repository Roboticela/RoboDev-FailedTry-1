#include "XmlToCpp.hpp"
#include "XmlParser.hpp"

#include <cstdlib>
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

        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << "scene.AddInput(std::move(input));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    return code.str();
}

std::string XmlToCpp::Convert(const std::string& xml) const {
    XmlParser parser;
    const XmlNode root = parser.Parse(xml);
    std::ostringstream code;
    code << "#include \"Robo/Text.hpp\"\n";
    code << "#include \"Robo/Heading.hpp\"\n";
    code << "#include \"Robo/Paragraph.hpp\"\n";
    code << "#include \"Robo/Button.hpp\"\n";
    code << "#include \"Robo/Input.hpp\"\n";
    code << "#include \"Robo/RoboScene.hpp\"\n";
    code << "#include \"Robo/Color.hpp\"\n";
    code << "#include \"Robo/FontStyle.hpp\"\n";
    code << "#include \"Robo/TextAlign.hpp\"\n\n";
    code << ConvertNode(root, 0);
    return code.str();
}

}  // namespace Robo
