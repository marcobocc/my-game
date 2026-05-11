#pragma once
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include "Asset.hpp"

class Material final : public Asset {
public:
    Material(const std::string& name,
             const std::string& shaderName,
             const glm::vec4& tint,
             const std::string& albedoTexture,
             const std::string& normalTexture,
             const std::string& roughnessTexture,
             const std::string& metallicTexture,
             const std::string& aoTexture,
             float metallic,
             float roughness,
             float ao,
             const glm::vec2& tiling = glm::vec2(1.0f, 1.0f),
             const glm::vec2& offset = glm::vec2(0.0f, 0.0f)) :
        Asset(name),
        shaderName_(shaderName),
        tint_(tint),
        albedoTexture_(albedoTexture),
        normalTexture_(normalTexture),
        roughnessTexture_(roughnessTexture),
        metallicTexture_(metallicTexture),
        aoTexture_(aoTexture),
        metallic_(metallic),
        roughness_(roughness),
        ao_(ao),
        tiling_(tiling),
        offset_(offset) {}
    const glm::vec2& getTiling() const { return tiling_; }
    const glm::vec2& getOffset() const { return offset_; }

    const std::string& getShaderName() const { return shaderName_; }
    const glm::vec4& getTint() const { return tint_; }
    const std::string& getAlbedoTexture() const { return albedoTexture_; }
    const std::string& getNormalTexture() const { return normalTexture_; }
    const std::string& getRoughnessTexture() const { return roughnessTexture_; }
    const std::string& getMetallicTexture() const { return metallicTexture_; }
    const std::string& getAoTexture() const { return aoTexture_; }
    float getMetallic() const { return metallic_; }
    float getRoughness() const { return roughness_; }
    float getAo() const { return ao_; }

private:
    std::string shaderName_;
    glm::vec4 tint_;
    std::string albedoTexture_;
    std::string normalTexture_;
    std::string roughnessTexture_;
    std::string metallicTexture_;
    std::string aoTexture_;
    float metallic_;
    float roughness_;
    float ao_;
    glm::vec2 tiling_;
    glm::vec2 offset_;
};
