#include "Robo/Sidebar.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

bool SidebarItemBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void SidebarItem::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& SidebarItem::GetLabel() const {
    return label_;
}

void SidebarItem::SetSection(bool section) {
    section_ = section;
}

bool SidebarItem::IsSection() const {
    return section_;
}

void SidebarItem::SetInteraction(SidebarItemInteraction interaction) {
    interaction_ = interaction;
}

SidebarItemInteraction SidebarItem::GetInteraction() const {
    return interaction_;
}

void SidebarItem::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool SidebarItem::IsDisabled() const {
    return disabled_;
}

void SidebarItem::SetLoading(bool loading) {
    loading_ = loading;
}

bool SidebarItem::IsLoading() const {
    return loading_;
}

bool SidebarItem::CanActivate() const {
    return !section_ && !disabled_ && !loading_;
}

void SidebarItem::SetFocused(bool focused) {
    focused_ = focused;
    if (!focused_) {
        focusVisible_ = false;
    }
}

bool SidebarItem::IsFocused() const {
    return focused_;
}

void SidebarItem::SetFocusVisible(bool visible) {
    focusVisible_ = visible && focused_;
}

bool SidebarItem::IsFocusVisible() const {
    return focusVisible_;
}

void SidebarItem::SetSelected(bool selected) {
    selected_ = selected;
}

bool SidebarItem::IsSelected() const {
    return selected_;
}

void SidebarItem::SetIcon(Svg icon) {
    icon_ = std::move(icon);
    hasIcon_ = true;
}

bool SidebarItem::HasIcon() const {
    return hasIcon_;
}

Svg& SidebarItem::Icon() {
    return icon_;
}

const Svg& SidebarItem::Icon() const {
    return icon_;
}

void SidebarItem::AddChild(SidebarItem child) {
    children_.push_back(std::move(child));
}

std::vector<SidebarItem>& SidebarItem::Children() {
    return children_;
}

const std::vector<SidebarItem>& SidebarItem::Children() const {
    return children_;
}

void SidebarItem::SetBounds(const SidebarItemBounds& bounds) {
    bounds_ = bounds;
}

const SidebarItemBounds& SidebarItem::GetBounds() const {
    return bounds_;
}

void Sidebar::SetVariant(SidebarVariant variant) {
    variant_ = variant;
}

SidebarVariant Sidebar::GetVariant() const {
    return variant_;
}

void Sidebar::AddItem(SidebarItem item) {
    items_.push_back(std::move(item));
}

std::vector<SidebarItem>& Sidebar::Items() {
    return items_;
}

const std::vector<SidebarItem>& Sidebar::Items() const {
    return items_;
}

void Sidebar::Select(int index) {
    if (items_.empty() || index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    if (items_[static_cast<std::size_t>(index)].IsSection()) {
        return;
    }
    selected_ = index;
    for (SidebarItem& item : items_) {
        item.SetSelected(false);
        for (SidebarItem& child : item.Children()) {
            child.SetSelected(false);
        }
    }
}

int Sidebar::IndexOf(const SidebarItem* item) const {
    if (!item) {
        return -1;
    }
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        if (&items_[static_cast<std::size_t>(i)] == item) {
            return i;
        }
    }
    return -1;
}

bool Sidebar::SelectChild(SidebarItem* child) {
    if (!child || !child->CanActivate()) {
        return false;
    }
    bool found = false;
    for (SidebarItem& item : items_) {
        for (SidebarItem& entry : item.Children()) {
            const bool on = &entry == child;
            found = found || on;
            entry.SetSelected(on);
        }
    }
    return found;
}

int Sidebar::Selected() const {
    return selected_;
}

int Sidebar::HitIndex(float x, float y) const {
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        const SidebarItem& item = items_[static_cast<std::size_t>(i)];
        if (!item.CanActivate()) {
            continue;
        }
        if (item.GetBounds().Contains(x, y)) {
            return i;
        }
    }
    return -1;
}

SidebarItem* Sidebar::HitItem(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SidebarItem& item = items_[static_cast<std::size_t>(i)];
        for (int c = static_cast<int>(item.Children().size()) - 1; c >= 0; --c) {
            SidebarItem& child = item.Children()[static_cast<std::size_t>(c)];
            if (child.CanActivate() && child.GetBounds().height > 1.0f && child.GetBounds().Contains(x, y) &&
                (viewH_ <= 1.0f || (y >= viewY_ && y < viewY_ + viewH_))) {
                return &child;
            }
        }
        if (item.CanActivate() && item.GetBounds().Contains(x, y)) {
            return &item;
        }
    }
    return nullptr;
}

void Sidebar::SetPanel(float paneX, float paneY, float paneWidth, float paneHeight, float viewY, float viewHeight, float contentHeight) {
    paneX_ = paneX;
    paneY_ = paneY;
    paneW_ = std::max(0.0f, paneWidth);
    paneH_ = std::max(0.0f, paneHeight);
    viewY_ = viewY;
    viewH_ = std::max(0.0f, viewHeight);
    contentHeight_ = std::max(0.0f, contentHeight);
    const float maxScroll = std::max(0.0f, contentHeight_ - viewH_);
    scroll_ = std::max(0.0f, std::min(scroll_, maxScroll));
}

bool Sidebar::Contains(float x, float y) const {
    return paneW_ > 1.0f && paneH_ > 1.0f && x >= paneX_ && y >= paneY_ && x < paneX_ + paneW_ && y < paneY_ + paneH_;
}

bool Sidebar::ScrollBy(float delta) {
    const float maxScroll = std::max(0.0f, contentHeight_ - viewH_);
    const float next = std::max(0.0f, std::min(maxScroll, scroll_ + delta));
    if (std::fabs(next - scroll_) < 0.5f) {
        return false;
    }
    scroll_ = next;
    return true;
}

void Sidebar::Reveal(float itemY, float itemHeight) {
    if (viewH_ <= 1.0f) {
        return;
    }
    if (itemY < viewY_) {
        scroll_ -= viewY_ - itemY;
    } else if (itemY + itemHeight > viewY_ + viewH_) {
        scroll_ += itemY + itemHeight - (viewY_ + viewH_);
    }
    const float maxScroll = std::max(0.0f, contentHeight_ - viewH_);
    scroll_ = std::max(0.0f, std::min(scroll_, maxScroll));
}

float Sidebar::Scroll() const {
    return scroll_;
}

float Sidebar::LayoutWidth(float openAmount) const {
    const float open = std::max(0.0f, std::min(1.0f, openAmount));
    if (items_.empty()) {
        return SIDEBAR_WIDTH * open;
    }
    return SIDEBAR_RAIL + (SIDEBAR_WIDTH - SIDEBAR_RAIL) * open;
}

SidebarVariant ParseSidebarVariant(const std::string& value) {
    if (value == "activity" || value == "dual" || value == "wide") {
        return SidebarVariant::Activity;
    }
    if (value == "options" || value == "normal" || value == "buttons") {
        return SidebarVariant::Normal;
    }
    return SidebarVariant::Normal;
}

}  // namespace Robo
