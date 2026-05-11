#pragma once
#include <glm/vec4.hpp>
#include <string>
#include "Asset.hpp"

class Material final : public Asset {
public:
    Material(const std::string& name,
             const std::string& shaderName,
             const glm::vec4& tint,
             const std::string& albedoTexture) :
        Asset(name),
        shaderName_(shaderName),
        tint_(tint),
        albedoTexture_(albedoTexture) {}

    const std::string& getShaderName() const { return shaderName_; }
    const glm::vec4& getTint() const { return tint_; }
    const std::string& getAlbedoTexture() const { return albedoTexture_; }

private:
    std::string shaderName_;
    glm::vec4 tint_;
    std::string albedoTexture_;
};
