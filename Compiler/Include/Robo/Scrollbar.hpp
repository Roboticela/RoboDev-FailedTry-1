#pragma once

#include "Color.hpp"

#include <string>

namespace Robo {

enum class ScrollbarOrientation {
    Vertical,
    Horizontal
};

enum class ScrollbarInteraction {
    Normal,
    Hovered,
    Dragging
};

struct ScrollbarStyle {
    ScrollbarOrientation orientation = ScrollbarOrientation::Vertical;
    double thickness = 8.0;
    double length = 220.0;
    double cornerRadius = 4.0;
    double minThumbLength = 24.0;
    Color trackColor = Color::FromRgba(0, 0, 0, 0);
    Color thumbColor = Color::FromHex("#C7C7CC");
    Color thumbHoverColor = Color::FromHex("#AEAEB2");
    Color thumbActiveColor = Color::FromHex("#8E8E93");
    double opacity = 1.0;
    bool hasCustomThumbColor = false;
    bool hasCustomThumbHoverColor = false;
    bool hasCustomThumbActiveColor = false;
    bool hasCustomTrackColor = false;
};

struct ScrollbarBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

class Scrollbar {
public:
    Scrollbar();

    void SetStyle(const ScrollbarStyle& style);
    const ScrollbarStyle& GetStyle() const;
    ScrollbarStyle& GetStyle();

    void SetOrientation(ScrollbarOrientation orientation);
    ScrollbarOrientation GetOrientation() const;

    void SetContentSize(double contentSize);
    double GetContentSize() const;

    void SetViewportSize(double viewportSize);
    double GetViewportSize() const;

    void SetValue(double value);
    double GetValue() const;
    void ScrollTo(double value);
    bool ScrollBy(double deltaRatio);

    void SetThumbRatio(double ratio);
    double GetThumbRatio() const;
    bool HasExplicitThumbRatio() const;

    void SetInteraction(ScrollbarInteraction interaction);
    ScrollbarInteraction GetInteraction() const;

    void SetBounds(const ScrollbarBounds& bounds);
    const ScrollbarBounds& GetBounds() const;
    void SetPixelScale(float pixelScale);
    float GetPixelScale() const;

    void ApplyVariantDefaults();

    double TrackLength() const;
    double CrossSize() const;
    double ResolveThumbLength(double trackLength) const;
    double ResolveThumbOffset(double trackLength) const;
    ScrollbarBounds ResolveThumbBounds() const;

    Color ResolveThumbColor() const;
    Color ResolveTrackColor() const;
    float ResolveOpacity() const;

    bool ThumbContains(float x, float y) const;

    void JumpTo(float x, float y);
    void BeginDrag(float x, float y);
    void UpdateDrag(float x, float y);
    void EndDrag();
    bool IsDragging() const;

private:
    double TrackCoordinate(float x, float y) const;
    double ResolveThumbRatio() const;

    ScrollbarStyle style_;
    ScrollbarInteraction interaction_ = ScrollbarInteraction::Normal;
    ScrollbarBounds bounds_;
    double value_ = 0.0;
    double contentSize_ = 0.0;
    double viewportSize_ = 0.0;
    double thumbRatio_ = 0.0;
    bool hasExplicitThumbRatio_ = false;
    double dragGrabOffset_ = 0.0;
    bool dragging_ = false;
    float pixelScale_ = 1.0f;
};

ScrollbarOrientation ParseScrollbarOrientation(const std::string& value);

}  // namespace Robo
