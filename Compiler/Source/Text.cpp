#include "Robo/Text.hpp"
#include "Robo/IPlatform.hpp"

namespace Robo {

Text::Text() = default;

void Text::SetStyle(const TextStyle& style) {
    style_ = style;
}

const TextStyle& Text::GetStyle() const {
    return style_;
}

void Text::ClearRuns() {
    runs_.clear();
}

void Text::AddRun(const TextRun& run) {
    runs_.push_back(run);
}

void Text::AddPlain(const std::string& content) {
    TextRun run;
    run.content = content;
    runs_.push_back(run);
}

const std::vector<TextRun>& Text::GetRuns() const {
    return runs_;
}

void Text::Mount(IPlatform& platform, const std::string& parentId) {
    elementId_ = platform.MountTextElement(parentId, *this);
}

void Text::Update(IPlatform& platform) {
    if (!elementId_.empty()) {
        platform.UpdateTextElement(elementId_, *this);
    }
}

void Text::Unmount(IPlatform& platform) {
    if (!elementId_.empty()) {
        platform.RemoveElement(elementId_);
        elementId_.clear();
    }
}

const std::string& Text::GetElementId() const {
    return elementId_;
}

}  // namespace Robo
