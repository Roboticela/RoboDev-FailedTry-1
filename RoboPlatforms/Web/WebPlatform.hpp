#pragma once

#include "Robo/IPlatform.hpp"

#include <string>

namespace Robo {

class WebPlatform : public IPlatform {
public:
    void ClearRoot(const std::string& rootId) override;
    std::string MountTextElement(const std::string& parentId, const Text& text) override;
    void UpdateTextElement(const std::string& elementId, const Text& text) override;
    void RemoveElement(const std::string& elementId) override;
    std::string MeasureText(const std::string& content, double fontSize, const std::string& fontWeight) override;

private:
    std::string BuildCss(const Text& text) const;
    std::string NextElementId();
    int nextId_ = 1;
};

}  // namespace Robo
