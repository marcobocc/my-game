#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Asset.hpp"

class Font final : public Asset {
public:
    struct Glyph {
        float u0, v0, u1, v1; // UV rect in atlas (normalized 0..1)
        float bearingX; // horizontal offset from cursor to glyph left edge
        float bearingY; // vertical offset from baseline to glyph top edge
        float advance; // horizontal advance to next cursor position
        int width, height; // glyph size in pixels
    };

    Font(const std::string& name,
         uint32_t atlasWidth,
         uint32_t atlasHeight,
         std::vector<uint8_t> atlasRgba,
         std::unordered_map<uint32_t, Glyph> glyphs,
         float ascent,
         float descent,
         float lineGap,
         float fontSize) :
        Asset(name),
        atlasWidth(atlasWidth),
        atlasHeight(atlasHeight),
        atlasRgba(std::move(atlasRgba)),
        glyphs(std::move(glyphs)),
        ascent(ascent),
        descent(descent),
        lineGap(lineGap),
        fontSize(fontSize) {}

    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    std::vector<uint8_t> atlasRgba; // RGBA8, alpha = coverage
    std::unordered_map<uint32_t, Glyph> glyphs; // codepoint → glyph metrics
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineGap = 0.0f;
    float fontSize = 0.0f; // point size used when baking
};
