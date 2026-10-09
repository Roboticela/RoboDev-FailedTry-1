#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace Robo {

namespace {

struct ResolvedRun {
    std::string content;
    float fontSize = 16.0f;
    Color color;
    FontFaceId faceId = FontFaceId::Regular;
    TextDecoration decoration = TextDecoration::None;
    float letterSpacing = 0.0f;
    float opacity = 1.0f;
};

struct GlyphSpan {
    std::string text;
    ResolvedRun style;
    float width = 0.0f;
};

struct LayoutLine {
    std::vector<GlyphSpan> spans;
    float width = 0.0f;
};

ResolvedRun ResolveRun(const TextRun& run, const TextStyle& style, const FontEngine& fonts) {
    ResolvedRun resolved;
    resolved.content = run.content;
    resolved.fontSize = static_cast<float>(run.hasFontSize ? run.fontSize : style.fontSize);
    resolved.color = run.hasColor ? run.color : style.color;
    const FontWeight weight = run.hasFontWeight ? run.fontWeight : style.fontWeight;
    const FontStyleKind fontStyle = run.hasFontStyle ? run.fontStyle : style.fontStyle;
    resolved.faceId = fonts.ResolveFace(weight, fontStyle);
    resolved.decoration = run.hasTextDecoration ? run.textDecoration : style.textDecoration;
    resolved.letterSpacing = static_cast<float>(run.hasLetterSpacing ? run.letterSpacing : style.letterSpacing);
    resolved.opacity = static_cast<float>(run.hasOpacity ? run.opacity : style.opacity);
    return resolved;
}

bool IsSpace(char ch) {
    return ch == ' ' || ch == '\t';
}

std::vector<std::string> SplitTokens(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '\n') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back("\n");
            continue;
        }
        if (IsSpace(ch)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, ch));
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

void PushSpan(LayoutLine& line, const GlyphSpan& span) {
    if (span.text.empty()) {
        return;
    }
    line.spans.push_back(span);
    line.width += span.width;
}

}  // namespace

float TextPainter::ResolveWidthValue(const std::string& value, float referenceWidth) {
    if (value.empty()) {
        return referenceWidth;
    }
    if (value == "100%") {
        return referenceWidth;
    }
    if (value.size() > 1 && value.back() == '%') {
        const float percent = static_cast<float>(std::atof(value.c_str()));
        return referenceWidth * (percent / 100.0f);
    }
    if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        return static_cast<float>(std::atof(value.c_str()) * 16.0);
    }
    if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        return static_cast<float>(std::atof(value.c_str()));
    }
    return static_cast<float>(std::atof(value.c_str()));
}

float TextPainter::Paint(FrameBuffer& buffer,
                         const FontEngine& fonts,
                         const Text& text,
                         float originX,
                         float originY,
                         float containerWidth) {
    const TextStyle& style = text.GetStyle();
    float blockWidth = ResolveWidthValue(style.width, containerWidth);
    if (!style.maxWidth.empty()) {
        blockWidth = std::min(blockWidth, ResolveWidthValue(style.maxWidth, containerWidth));
    }
    blockWidth = std::max(0.0f, blockWidth);

    std::vector<ResolvedRun> runs;
    for (const TextRun& run : text.GetRuns()) {
        runs.push_back(ResolveRun(run, style, fonts));
    }

    std::vector<LayoutLine> lines;
    LayoutLine currentLine;

    auto BreakLongToken = [&](const std::string& token, const ResolvedRun& runStyle) {
        std::string built;
        for (char ch : token) {
            std::string candidate = built + ch;
            const float width = fonts.MeasureWidth(candidate, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            if (!built.empty() && style.wrap && currentLine.width + width > blockWidth && !currentLine.spans.empty()) {
                GlyphSpan span;
                span.text = built;
                span.style = runStyle;
                span.width = fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
                PushSpan(currentLine, span);
                lines.push_back(currentLine);
                currentLine = LayoutLine();
                built.assign(1, ch);
            } else {
                built = candidate;
            }
        }
        if (!built.empty()) {
            GlyphSpan span;
            span.text = built;
            span.style = runStyle;
            span.width = fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            if (style.wrap && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
            }
            PushSpan(currentLine, span);
        }
    };

    for (const ResolvedRun& run : runs) {
        const std::vector<std::string> tokens = SplitTokens(run.content);
        for (const std::string& token : tokens) {
            if (token == "\n") {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
                continue;
            }

            GlyphSpan span;
            span.text = token;
            span.style = run;
            span.width = fonts.MeasureWidth(token, run.fontSize, run.faceId, run.letterSpacing);

            if (!style.wrap) {
                PushSpan(currentLine, span);
                continue;
            }

            if (!currentLine.spans.empty() && currentLine.width + span.width > blockWidth && !IsSpace(token[0])) {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
            }

            if (span.width > blockWidth && !IsSpace(token[0])) {
                BreakLongToken(token, run);
            } else {
                PushSpan(currentLine, span);
            }
        }
    }
    lines.push_back(currentLine);

    float maxAscent = 0.0f;
    float maxDescent = 0.0f;
    for (const ResolvedRun& run : runs) {
        maxAscent = std::max(maxAscent, fonts.GetAscent(run.fontSize, run.faceId));
        maxDescent = std::max(maxDescent, fonts.GetDescent(run.fontSize, run.faceId));
    }
    if (maxAscent <= 0.0f) {
        maxAscent = static_cast<float>(style.fontSize) * 0.8f;
    }
    const float baseLineHeight = (maxAscent + maxDescent) * static_cast<float>(style.lineHeight);
    float cursorY = originY;

    for (LayoutLine& line : lines) {
        while (!line.spans.empty() && !line.spans.front().text.empty() && IsSpace(line.spans.front().text[0])) {
            line.width -= line.spans.front().width;
            line.spans.erase(line.spans.begin());
        }
        while (!line.spans.empty() && !line.spans.back().text.empty() && IsSpace(line.spans.back().text[0])) {
            line.width -= line.spans.back().width;
            line.spans.pop_back();
        }

        float startX = originX;
        float extraGap = 0.0f;
        int gapCount = 0;
        if (style.align == TextAlign::Center) {
            startX = originX + (blockWidth - line.width) * 0.5f;
        } else if (style.align == TextAlign::Right) {
            startX = originX + (blockWidth - line.width);
        } else if (style.align == TextAlign::Justify && &line != &lines.back()) {
            for (const GlyphSpan& span : line.spans) {
                if (!span.text.empty() && IsSpace(span.text[0])) {
                    ++gapCount;
                }
            }
            if (gapCount > 0 && blockWidth > line.width) {
                extraGap = (blockWidth - line.width) / static_cast<float>(gapCount);
            }
        }

        float penX = startX;
        const float baseline = cursorY + maxAscent;

        for (const GlyphSpan& span : line.spans) {
            fonts.DrawString(buffer,
                             penX,
                             baseline,
                             span.text,
                             span.style.fontSize,
                             span.style.faceId,
                             span.style.color,
                             span.style.opacity * static_cast<float>(style.opacity),
                             span.style.letterSpacing);

            if (span.style.decoration == TextDecoration::Underline) {
                const int y = static_cast<int>(std::round(baseline + std::max(1.0f, span.style.fontSize * 0.08f)));
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                y,
                                static_cast<int>(std::ceil(span.width)),
                                std::max(1, static_cast<int>(span.style.fontSize * 0.06f)),
                                span.style.color,
                                span.style.opacity * static_cast<float>(style.opacity));
            } else if (span.style.decoration == TextDecoration::LineThrough) {
                const int y = static_cast<int>(std::round(baseline - maxAscent * 0.35f));
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                y,
                                static_cast<int>(std::ceil(span.width)),
                                std::max(1, static_cast<int>(span.style.fontSize * 0.06f)),
                                span.style.color,
                                span.style.opacity * static_cast<float>(style.opacity));
            }

            penX += span.width;
            if (!span.text.empty() && IsSpace(span.text[0])) {
                penX += extraGap;
            }
        }

        cursorY += baseLineHeight;
    }

    return cursorY - originY + 12.0f;
}

}  // namespace Robo
