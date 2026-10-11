#pragma once

#include "SvgPath.hpp"
#include "XmlNode.hpp"

#include <map>
#include <sstream>
#include <string>

namespace Robo {

class XmlToCpp {
public:
    std::string Convert(const std::string& xml, const std::string& assetDirectory) const;

private:
    std::string ConvertNode(const XmlNode& node, int indent) const;
    void EmitScrollbar(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitContainer(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitFlexbox(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitFlexChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitGrid(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitGridChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitPosition(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitPositionChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    // Normalises a numeric XML attribute into a valid C++ float literal.
    void EmitSelectable(std::ostringstream& code, int indent, const std::string& styleName, const XmlNode& node) const;
    bool TryEmitGraphic(std::ostringstream& code, const XmlNode& node, int indent, const std::string& parent, bool withProps) const;
    std::string FloatLiteral(const std::string& value) const;
    std::string Indent(int level) const;
    std::string EscapeCpp(const std::string& value) const;

    mutable std::map<std::string, CompiledSvg> icons_;
};

}  // namespace Robo
