#include "Robo/Scrollbar.hpp"

#include <algorithm>

namespace Robo {

namespace {

double Clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

}  // namespace

bool ScrollbarBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

Scrollbar::Scrollbar() = default;

void Scrollbar::SetStyle(const ScrollbarStyle& style) {
    style_ = style;
}

const ScrollbarStyle& Scrollbar::GetStyle() const {
    return style_;
}

ScrollbarStyle& Scrollbar::GetStyle() {
    return style_;
}

void Scrollbar::SetOrientation(ScrollbarOrientation orientation) {
    style_.orientation = orientation;
}

ScrollbarOrientation Scrollbar::GetOrientation() const {
    return style_.orientation;
}

void Scrollbar::SetContentSize(double contentSize) {
    contentSize_ = std::max(0.0, contentSize);
}

double Scrollbar::GetContentSize() const {
    return contentSize_;
}

void Scrollbar::SetViewportSize(double viewportSize) {
    viewportSize_ = std::max(0.0, viewportSize);
}

double Scrollbar::GetViewportSize() const {
    return viewportSize_;
}

void Scrollbar::SetValue(double value) {
    value_ = Clamp01(value);
}

double Scrollbar::GetValue() const {
    return value_;
}

void Scrollbar::ScrollTo(double value) {
    SetValue(value);
}

bool Scrollbar::ScrollBy(double deltaRatio) {
    const double next = Clamp01(value_ + deltaRatio);
    if (next == value_) {
        return false;
    }
    value_ = next;
    return true;
}

void Scrollbar::SetThumbRatio(double ratio) {
    thumbRatio_ = Clamp01(ratio);
    hasExplicitThumbRatio_ = true;
}

double Scrollbar::GetThumbRatio() const {
    return ResolveThumbRatio();
}

bool Scrollbar::HasExplicitThumbRatio() const {
    return hasExplicitThumbRatio_;
}

void Scrollbar::SetInteraction(ScrollbarInteraction interaction) {
    interaction_ = interaction;
}

ScrollbarInteraction Scrollbar::GetInteraction() const {
    return interaction_;
}

void Scrollbar::SetBounds(const ScrollbarBounds& bounds) {
    bounds_ = bounds;
}

const ScrollbarBounds& Scrollbar::GetBounds() const {
    return bounds_;
}

void Scrollbar::SetPixelScale(float pixelScale) {
    pixelScale_ = std::max(0.5f, pixelScale);
}

float Scrollbar::GetPixelScale() const {
    return pixelScale_;
}

void Scrollbar::ApplyVariantDefaults() {
    if (!style_.hasCustomThumbColor) {
        style_.thumbColor = Color::FromHex("#C7C7CC");
    }
    // When a custom thumb color is supplied without explicit interaction colors,
    // reuse it so hover/drag never fall back to a clashing default gray.
    if (!style_.hasCustomThumbHoverColor) {
        style_.thumbHoverColor =
            style_.hasCustomThumbColor ? style_.thumbColor : Color::FromHex("#AEAEB2");
    }
    if (!style_.hasCustomThumbActiveColor) {
        style_.thumbActiveColor =
            style_.hasCustomThumbColor ? style_.thumbColor : Color::FromHex("#8E8E93");
    }
    if (style_.cornerRadius <= 0.0) {
        style_.cornerRadius = style_.thickness * 0.5;
    }
}

double Scrollbar::TrackLength() const {
    return (style_.orientation == ScrollbarOrientation::Vertical) ? static_cast<double>(bounds_.height)
                                                                  : static_cast<double>(bounds_.width);
}

double Scrollbar::CrossSize() const {
    return (style_.orientation == ScrollbarOrientation::Vertical) ? static_cast<double>(bounds_.width)
                                                                  : static_cast<double>(bounds_.height);
}

double Scrollbar::ResolveThumbRatio() const {
    if (hasExplicitThumbRatio_) {
        return Clamp01(thumbRatio_);
    }
    if (contentSize_ <= 0.0 || viewportSize_ <= 0.0) {
        return 1.0;
    }
    if (contentSize_ <= viewportSize_) {
        return 1.0;
    }
    return Clamp01(viewportSize_ / contentSize_);
}

double Scrollbar::ResolveThumbLength(double trackLength) const {
    if (trackLength <= 0.0) {
        return 0.0;
    }
    const double minLength =
        std::min(static_cast<double>(style_.minThumbLength) * static_cast<double>(pixelScale_), trackLength);
    const double length = ResolveThumbRatio() * trackLength;
    return std::max(minLength, std::min(length, trackLength));
}

double Scrollbar::ResolveThumbOffset(double trackLength) const {
    const double thumbLength = ResolveThumbLength(trackLength);
    const double maxStart = std::max(0.0, trackLength - thumbLength);
    return Clamp01(value_) * maxStart;
}

ScrollbarBounds Scrollbar::ResolveThumbBounds() const {
    ScrollbarBounds thumb;
    const double trackLength = TrackLength();
    const double thumbLength = ResolveThumbLength(trackLength);
    const double thumbOffset = ResolveThumbOffset(trackLength);

    if (style_.orientation == ScrollbarOrientation::Vertical) {
        thumb.x = bounds_.x;
        thumb.y = bounds_.y + static_cast<float>(thumbOffset);
        thumb.width = bounds_.width;
        thumb.height = static_cast<float>(thumbLength);
    } else {
        thumb.x = bounds_.x + static_cast<float>(thumbOffset);
        thumb.y = bounds_.y;
        thumb.width = static_cast<float>(thumbLength);
        thumb.height = bounds_.height;
    }
    return thumb;
}

Color Scrollbar::ResolveThumbColor() const {
    if (interaction_ == ScrollbarInteraction::Dragging) {
        return style_.thumbActiveColor;
    }
    if (interaction_ == ScrollbarInteraction::Hovered) {
        return style_.thumbHoverColor;
    }
    return style_.thumbColor;
}

Color Scrollbar::ResolveTrackColor() const {
    return style_.trackColor;
}

float Scrollbar::ResolveOpacity() const {
    return static_cast<float>(style_.opacity);
}

bool Scrollbar::ThumbContains(float x, float y) const {
    return ResolveThumbBounds().Contains(x, y);
}

double Scrollbar::TrackCoordinate(float x, float y) const {
    if (style_.orientation == ScrollbarOrientation::Vertical) {
        return static_cast<double>(y) - static_cast<double>(bounds_.y);
    }
    return static_cast<double>(x) - static_cast<double>(bounds_.x);
}

void Scrollbar::JumpTo(float x, float y) {
    BeginDrag(x, y);
}

void Scrollbar::BeginDrag(float x, float y) {
    const double trackLength = TrackLength();
    const double thumbLength = ResolveThumbLength(trackLength);
    const double maxStart = std::max(0.0, trackLength - thumbLength);
    const double coordinate = TrackCoordinate(x, y);

    double start = ResolveThumbOffset(trackLength);
    if (!ThumbContains(x, y) && maxStart > 0.0) {
        start = std::max(0.0, std::min(coordinate - thumbLength * 0.5, maxStart));
        value_ = start / maxStart;
    }

    dragGrabOffset_ = coordinate - start;
    dragging_ = true;
    interaction_ = ScrollbarInteraction::Dragging;
}

void Scrollbar::UpdateDrag(float x, float y) {
    if (!dragging_) {
        return;
    }
    const double trackLength = TrackLength();
    const double thumbLength = ResolveThumbLength(trackLength);
    const double maxStart = std::max(0.0, trackLength - thumbLength);
    if (maxStart <= 0.0) {
        return;
    }
    const double coordinate = TrackCoordinate(x, y);
    const double start = std::max(0.0, std::min(coordinate - dragGrabOffset_, maxStart));
    value_ = start / maxStart;
}

void Scrollbar::EndDrag() {
    dragging_ = false;
    if (interaction_ == ScrollbarInteraction::Dragging) {
        interaction_ = ScrollbarInteraction::Normal;
    }
}

bool Scrollbar::IsDragging() const {
    return dragging_;
}

ScrollbarOrientation ParseScrollbarOrientation(const std::string& value) {
    if (value == "horizontal" || value == "h" || value == "row") {
        return ScrollbarOrientation::Horizontal;
    }
    return ScrollbarOrientation::Vertical;
}

}  // namespace Robo
