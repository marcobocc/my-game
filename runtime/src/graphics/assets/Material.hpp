#pragma once
#include <glm/vec2.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "../../core/assets/Asset.hpp"
#include "core/assets/BuiltinAssetNames.hpp"
#include "utils/JsonUtils.hpp"

class Material final : public Asset {
public:
    Material(const std::string& name,
             const glm::vec4& tint,
             const std::string& albedoTexture,
             const std::string& normalTexture,
             const std::string& roughnessTexture,
             const std::string& metallicTexture,
             const std::string& aoTexture,
             float metallic,
             float roughness,
             float ao,
             const glm::vec2& tiling,
             const glm::vec2& offset,
             bool scaleInvariantUV,
             const std::string& splatMapTexture = EMPTY_TEXTURE,
             const std::string& layerMaterial0 = SOLID_COLOR_MATERIAL,
             const std::string& layerMaterial1 = SOLID_COLOR_MATERIAL,
             const std::string& layerMaterial2 = SOLID_COLOR_MATERIAL,
             const std::string& layerMaterial3 = SOLID_COLOR_MATERIAL) :
        Asset(name),
        tint(tint),
        albedoTexture(albedoTexture),
        normalTexture(normalTexture),
        roughnessTexture(roughnessTexture),
        metallicTexture(metallicTexture),
        aoTexture(aoTexture),
        metallic(metallic),
        roughness(roughness),
        ao(ao),
        tiling(tiling),
        offset(offset),
        scaleInvariantUV(scaleInvariantUV),
        splatMapTexture(splatMapTexture),
        layerMaterial0(layerMaterial0),
        layerMaterial1(layerMaterial1),
        layerMaterial2(layerMaterial2),
        layerMaterial3(layerMaterial3) {}

    // A material paints via a splatmap once "Enable Painting" has assigned one (see terrain.md Milestone 3);
    // this is what routes it to the terrain-blend shader/descriptor path instead of the single-albedo path.
    bool isTerrainBlend() const { return splatMapTexture != EMPTY_TEXTURE; }

    static Material deserialize(const nlohmann::json& j, const std::string& name) {
        return {name,
                JsonUtils::getOptional<glm::vec4>(j, "tint", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}),
                JsonUtils::getOptional<std::string>(j, "albedoTexture", EMPTY_TEXTURE),
                JsonUtils::getOptional<std::string>(j, "normalTexture", EMPTY_TEXTURE),
                JsonUtils::getOptional<std::string>(j, "roughnessTexture", EMPTY_TEXTURE),
                JsonUtils::getOptional<std::string>(j, "metallicTexture", EMPTY_TEXTURE),
                JsonUtils::getOptional<std::string>(j, "aoTexture", EMPTY_TEXTURE),
                JsonUtils::getOptional<float>(j, "metallic", 0.0f),
                JsonUtils::getOptional<float>(j, "roughness", 1.0f),
                JsonUtils::getOptional<float>(j, "ao", 1.0f),
                JsonUtils::getOptional<glm::vec2>(j, "tiling", glm::vec2{1.0f, 1.0f}),
                JsonUtils::getOptional<glm::vec2>(j, "offset", glm::vec2{0.0f, 0.0f}),
                JsonUtils::getOptional<bool>(j, "scaleInvariantUV", false),
                JsonUtils::getOptional<std::string>(j, "splatMapTexture", EMPTY_TEXTURE),
                JsonUtils::getOptional<std::string>(j, "layerMaterial0", SOLID_COLOR_MATERIAL),
                JsonUtils::getOptional<std::string>(j, "layerMaterial1", SOLID_COLOR_MATERIAL),
                JsonUtils::getOptional<std::string>(j, "layerMaterial2", SOLID_COLOR_MATERIAL),
                JsonUtils::getOptional<std::string>(j, "layerMaterial3", SOLID_COLOR_MATERIAL)};
    }

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["tint"] = JsonUtils::serializeVec4(tint);
        j["albedoTexture"] = albedoTexture;
        j["normalTexture"] = normalTexture;
        j["roughnessTexture"] = roughnessTexture;
        j["metallicTexture"] = metallicTexture;
        j["aoTexture"] = aoTexture;
        j["metallic"] = metallic;
        j["roughness"] = roughness;
        j["ao"] = ao;
        j["tiling"] = JsonUtils::serializeVec2(tiling);
        j["offset"] = JsonUtils::serializeVec2(offset);
        j["scaleInvariantUV"] = scaleInvariantUV;
        j["splatMapTexture"] = splatMapTexture;
        j["layerMaterial0"] = layerMaterial0;
        j["layerMaterial1"] = layerMaterial1;
        j["layerMaterial2"] = layerMaterial2;
        j["layerMaterial3"] = layerMaterial3;
        return j;
    }

    glm::vec4 tint;
    std::string albedoTexture;
    std::string normalTexture;
    std::string roughnessTexture;
    std::string metallicTexture;
    std::string aoTexture;
    float metallic;
    float roughness;
    float ao;
    glm::vec2 tiling;
    glm::vec2 offset;
    bool scaleInvariantUV;
    std::string splatMapTexture;
    // Terrain-blend layers: each references another Material asset whose albedo/tint/tiling
    // define the layer's appearance. Weighted by the splatmap's RGBA channels.
    std::string layerMaterial0;
    std::string layerMaterial1;
    std::string layerMaterial2;
    std::string layerMaterial3;
};
