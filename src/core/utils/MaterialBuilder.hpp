#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "core/AssetManager.hpp"
#include "core/assets/types/Material.hpp"

class MaterialBuilder {
public:
    explicit MaterialBuilder(AssetManager& assetManager) : assetManager_(assetManager) {}

    std::string createMaterialColor(const glm::vec4& color) const {
        std::string materialName = getMaterialNameForColor(color);

        auto material = std::make_unique<Material>();
        material->name = materialName;
        material->shaderPipelineName = DEFAULT_MATERIAL_SHADER_PIPELINE;
        material->baseColor = color;

        assetManager_.insertMaterial(std::move(material));
        return materialName;
    }

private:
    static constexpr auto DEFAULT_MATERIAL_SHADER_PIPELINE = "uniform_coloring";

    static constexpr auto MATERIAL_COLOR_WHITE = "_material_color_white";
    static constexpr auto MATERIAL_COLOR_RED = "_material_color_red";
    static constexpr auto MATERIAL_COLOR_GREEN = "_material_color_green";
    static constexpr auto MATERIAL_COLOR_BLUE = "_material_color_blue";
    static constexpr auto MATERIAL_COLOR_BLACK = "_material_color_black";
    static constexpr auto MATERIAL_COLOR_YELLOW = "_material_color_yellow";
    static constexpr auto MATERIAL_COLOR_CYAN = "_material_color_cyan";
    static constexpr auto MATERIAL_COLOR_MAGENTA = "_material_color_magenta";
    static constexpr auto MATERIAL_COLOR_ORANGE = "_material_color_orange";

    AssetManager& assetManager_;

    static std::string getMaterialNameForColor(const glm::vec4& color) {
        // Check for common colors and return predefined constants
        if (isColorClose(color, {1.0f, 1.0f, 1.0f, 1.0f})) return MATERIAL_COLOR_WHITE;
        if (isColorClose(color, {1.0f, 0.0f, 0.0f, 1.0f})) return MATERIAL_COLOR_RED;
        if (isColorClose(color, {0.0f, 1.0f, 0.0f, 1.0f})) return MATERIAL_COLOR_GREEN;
        if (isColorClose(color, {0.0f, 0.0f, 1.0f, 1.0f})) return MATERIAL_COLOR_BLUE;
        if (isColorClose(color, {0.0f, 0.0f, 0.0f, 1.0f})) return MATERIAL_COLOR_BLACK;
        if (isColorClose(color, {1.0f, 1.0f, 0.0f, 1.0f})) return MATERIAL_COLOR_YELLOW;
        if (isColorClose(color, {0.0f, 1.0f, 1.0f, 1.0f})) return MATERIAL_COLOR_CYAN;
        if (isColorClose(color, {1.0f, 0.0f, 1.0f, 1.0f})) return MATERIAL_COLOR_MAGENTA;
        if (isColorClose(color, {1.0f, 0.5f, 0.0f, 1.0f})) return MATERIAL_COLOR_ORANGE;

        // For non-standard colors, generate a unique name based on color values
        return "_material_color_" + std::to_string(static_cast<int>(color.r * 255)) + "_" +
               std::to_string(static_cast<int>(color.g * 255)) + "_" + std::to_string(static_cast<int>(color.b * 255));
    }

    static bool isColorClose(const glm::vec4& a, const glm::vec4& b, float epsilon = 0.01f) {
        return glm::length(a - b) < epsilon;
    }
};
