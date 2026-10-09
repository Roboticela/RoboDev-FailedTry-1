#pragma once

#include "XmlNode.hpp"

#include <string>

namespace Robo {

class XmlToCpp {
public:
    std::string Convert(const std::string& xml) const;

private:
    std::string ConvertNode(const XmlNode& node, int indent) const;
    std::string Indent(int level) const;
    std::string EscapeCpp(const std::string& value) const;
};

}  // namespace Robo
