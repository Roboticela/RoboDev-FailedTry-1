#pragma once

#include "XmlNode.hpp"
#include "Robo/IPlatform.hpp"
#include "Robo/Text.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Robo {

class XmlRuntime {
public:
    explicit XmlRuntime(IPlatform& platform);

    bool RenderXml(const std::string& xml, const std::string& rootId);
    const std::string& GetLastError() const;

private:
    void ClearMounted();
    void MountNode(const XmlNode& node, const std::string& parentId);
    Text BuildText(const XmlNode& node);
    void ApplyTextAttributes(const XmlNode& node, TextStyle& style) const;
    void ApplyRunAttributes(const XmlNode& node, TextRun& run) const;
    void CollectRuns(const XmlNode& node, const TextStyle& baseStyle, std::vector<TextRun>& runs);

    IPlatform& platform_;
    std::vector<std::unique_ptr<Text>> mountedTexts_;
    std::string lastError_;
};

}  // namespace Robo
