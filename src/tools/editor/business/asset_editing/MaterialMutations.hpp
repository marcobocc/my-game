#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "../ObjectSelection.hpp"
#include "EditorAssetRepository.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/asset_management/assets/Material.hpp"

class MaterialMutations {
public:
    MaterialMutations(EditorAssetRepository& repository, ObjectSelection& objectSelection) :
        repository_(repository),
        objectSelection_(objectSelection) {}

    AssetHandle createNew(const std::filesystem::path& relativePath = "") {
        std::string materialName = relativePath.empty() ? generateName() : relativePath.string();
        Material material(materialName,
                          GBUFFER_SHADER,
                          glm::vec4(1.0f),
                          EMPTY_TEXTURE,
                          EMPTY_TEXTURE,
                          EMPTY_TEXTURE,
                          EMPTY_TEXTURE,
                          EMPTY_TEXTURE,
                          0.0f,
                          1.0f,
                          1.0f,
                          glm::vec2(1.0f),
                          glm::vec2(0.0f),
                          false);

        repository_.create<Material>(materialName, material);
        objectSelection_.selectAsset(materialName);
        return materialName;
    }

    void deleteMaterial(const std::filesystem::path& relativePath) {
        std::string materialName = relativePath.string();
        if (!repository_.isMutable(materialName)) return;
        repository_.remove<Material>(materialName);
        clearSelectionIfSelected(materialName);
    }

    void setTint(const std::string& materialName, const glm::vec4& tint) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.tint = tint; });
    }

    void setShader(const std::string& materialName, const std::string& shaderName) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.shaderName = shaderName; });
    }

    void setAlbedoTexture(const std::string& materialName, const std::string& texture) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.albedoTexture = texture; });
    }

    void setNormalTexture(const std::string& materialName, const std::string& texture) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.normalTexture = texture; });
    }

    void setRoughnessTexture(const std::string& materialName, const std::string& texture) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.roughnessTexture = texture; });
    }

    void setMetallicTexture(const std::string& materialName, const std::string& texture) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.metallicTexture = texture; });
    }

    void setAoTexture(const std::string& materialName, const std::string& texture) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.aoTexture = texture; });
    }

    void setMetallic(const std::string& materialName, float metallic) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.metallic = metallic; });
    }

    void setRoughness(const std::string& materialName, float roughness) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.roughness = roughness; });
    }

    void setAo(const std::string& materialName, float ao) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.ao = ao; });
    }

    void setScaleInvariantUV(const std::string& materialName, bool scaleInvariantUV) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.scaleInvariantUV = scaleInvariantUV; });
    }

    void setTiling(const std::string& materialName, const glm::vec2& tiling) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.tiling = tiling; });
    }

    void setOffset(const std::string& materialName, const glm::vec2& offset) {
        repository_.mutate<Material>(materialName, [&](Material& mat) { mat.offset = offset; });
    }

    bool isMutable(const std::string& materialName) const { return repository_.isMutable(materialName); }

private:
    EditorAssetRepository& repository_;
    ObjectSelection& objectSelection_;

    std::string generateName() const {
        auto availableMaterials = repository_.list(".mat");
        int counter = 1;
        std::string materialName = "New Material (" + std::to_string(counter) + ").mat";
        while (std::ranges::find(availableMaterials, materialName) != availableMaterials.end()) {
            counter++;
            materialName = "New Material (" + std::to_string(counter) + ").mat";
        }
        return materialName;
    }

    void clearSelectionIfSelected(const std::string& materialName) const {
        auto selected = objectSelection_.getSelectedAssetId();
        if (selected && *selected == materialName) objectSelection_.clearSelection();
    }
};
