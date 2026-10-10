#pragma once

#include "XmlNode.hpp"

#include <string>

namespace Robo {

class XmlToCpp {
public:
    std::string Convert(const std::string& xml) const;

private:
    std::string ConvertNode(const XmlNode& node, int indent) const;
    void EmitFlexbox(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitFlexChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitGrid(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitGridChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitPosition(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitPositionChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    std::string Indent(int level) const;
    std::string EscapeCpp(const std::string& value) const;
};

}  // namespace Robo
