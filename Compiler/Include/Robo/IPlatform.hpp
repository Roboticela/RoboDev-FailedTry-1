#pragma once

#include "Text.hpp"

#include <string>

namespace Robo {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual void ClearRoot(const std::string& rootId) = 0;
    virtual std::string MountTextElement(const std::string& parentId, const Text& text) = 0;
    virtual void UpdateTextElement(const std::string& elementId, const Text& text) = 0;
    virtual void RemoveElement(const std::string& elementId) = 0;
    virtual std::string MeasureText(const std::string& content, double fontSize, const std::string& fontWeight) = 0;
};

}  // namespace Robo
