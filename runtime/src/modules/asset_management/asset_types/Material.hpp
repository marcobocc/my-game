#pragma once
#include <glm/vec2.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "Asset.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
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
             bool scaleInvariantUV) :
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
        scaleInvariantUV(scaleInvariantUV) {}

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
                JsonUtils::getOptional<bool>(j, "scaleInvariantUV", false)};
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
};
