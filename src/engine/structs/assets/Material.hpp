#pragma once
#include <glm/vec4.hpp>
#include <string>
#include "Asset.hpp"

class Material final : public Asset {
public:
    Material(const std::string& name,
             const std::string& shaderName,
             const glm::vec4& baseColor,
             const std::string& albedoTexture) :
        Asset(name),
        shaderName_(shaderName),
        baseColor_(baseColor),
        albedoTexture_(albedoTexture) {}

    const std::string& getShaderName() const { return shaderName_; }
    const glm::vec4& getBaseColor() const { return baseColor_; }
    const std::string& getAlbedoTexture() const { return albedoTexture_; }

private:
    std::string shaderName_;
    glm::vec4 baseColor_;
    std::string albedoTexture_;
};
