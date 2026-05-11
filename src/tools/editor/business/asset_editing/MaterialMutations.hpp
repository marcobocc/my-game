#pragma once
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include "../ObjectSelection.hpp"
#include "../UndoHistory.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/assets/BuiltinAssetNames.hpp"
#include "structs/assets/Material.hpp"
#include "utils/JsonUtils.hpp"

class MaterialMutations {
public:
    MaterialMutations(AssetManager& assetManager, ObjectSelection& objectSelection, UndoHistory& undoHistory) :
        assetManager_(assetManager),
        objectSelection_(objectSelection),
        undoHistory_(undoHistory) {}

    AssetHandle createNew(const std::filesystem::path& relativePath = "") const {
        std::string materialName;
        if (relativePath.empty()) {
            materialName = generateName();
        } else {
            materialName = relativePath.string();
        }
        Material defaultMaterial(materialName,
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
        nlohmann::json snapshot = serializeMaterial(defaultMaterial);
        upsertMaterial(materialName, snapshot);
        undoHistory_.push([this, materialName] { deleteMaterial_Command(materialName); },
                          [this, materialName, snapshot] { upsertMaterial(materialName, snapshot); });
        objectSelection_.selectAsset(materialName);
        return materialName;
    }

    void deleteMaterial(const std::filesystem::path& relativePath) const {
        std::string materialName = relativePath.string();
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json snapshot = serializeMaterial(*mat);
            deleteMaterial_Command(relativePath);
            undoHistory_.push([this, relativePath, snapshot] { upsertMaterial(relativePath, snapshot); },
                              [this, relativePath] { deleteMaterial_Command(relativePath); });
            clearSelectionIfSelected(materialName);
        }
    }

    void setTint(const std::string& materialName, const glm::vec4& color) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["tint"] = JsonUtils::serializeVec4(color);
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setAlbedoTexture(const std::string& materialName, const std::string& albedoTexture) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["albedoTexture"] = albedoTexture;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setShader(const std::string& materialName, const std::string& shaderName) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["shaderName"] = shaderName;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setNormalTexture(const std::string& materialName, const std::string& normalTexture) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["normalTexture"] = normalTexture;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setRoughnessTexture(const std::string& materialName, const std::string& roughnessTexture) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["roughnessTexture"] = roughnessTexture;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setMetallicTexture(const std::string& materialName, const std::string& metallicTexture) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["metallicTexture"] = metallicTexture;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setAoTexture(const std::string& materialName, const std::string& aoTexture) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["aoTexture"] = aoTexture;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setMetallic(const std::string& materialName, float metallic) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["metallic"] = metallic;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setRoughness(const std::string& materialName, float roughness) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["roughness"] = roughness;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setAo(const std::string& materialName, float ao) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["ao"] = ao;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setScaleInvariantUV(const std::string& materialName, int scaleInvariantUV) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["scaleInvariantUV"] = scaleInvariantUV;
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setTiling(const std::string& materialName, const glm::vec2& tiling) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["tiling"] = JsonUtils::serializeVec2(tiling);
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }
    void setOffset(const std::string& materialName, const glm::vec2& offset) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["offset"] = JsonUtils::serializeVec2(offset);
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void flushDirtyMaterials() {
        for (const auto& [name, snapshot]: dirtyMaterials_) {
            saveMaterial(name, snapshot);
        }
        dirtyMaterials_.clear();
    }

    bool isBuiltin(const std::string& materialName) const { return assetManager_.isBuiltin(materialName); }

private:
    AssetManager& assetManager_;
    ObjectSelection& objectSelection_;
    UndoHistory& undoHistory_;
    mutable std::unordered_map<std::string, nlohmann::json> dirtyMaterials_;

    std::string generateName() const {
        auto availableMaterials = assetManager_.getAvailableAssets(".mat");
        int counter = 1;
        std::string materialName = "New Material (" + std::to_string(counter) + ").mat";
        while (std::ranges::find(availableMaterials, materialName) != availableMaterials.end()) {
            counter++;
            materialName = "New Material (" + std::to_string(counter) + ").mat";
        }
        return materialName;
    }

    static nlohmann::json serializeMaterial(const Material& material) {
        nlohmann::json j;
        j["shaderName"] = material.getShaderName();
        j["tint"] = JsonUtils::serializeVec4(material.getTint());
        j["albedoTexture"] = material.getAlbedoTexture();
        j["normalTexture"] = material.getNormalTexture();
        j["roughnessTexture"] = material.getRoughnessTexture();
        j["metallicTexture"] = material.getMetallicTexture();
        j["aoTexture"] = material.getAoTexture();
        j["metallic"] = material.getMetallic();
        j["roughness"] = material.getRoughness();
        j["ao"] = material.getAo();
        j["tiling"] = JsonUtils::serializeVec2(material.getTiling());
        j["offset"] = JsonUtils::serializeVec2(material.getOffset());
        j["scaleInvariantUV"] = material.getScaleInvariantUV();
        return j;
    }

    void updateMaterial(const std::string& materialName,
                        const nlohmann::json& oldSnapshot,
                        const nlohmann::json& newSnapshot) const {
        applyMaterialSnapshot(materialName, newSnapshot);
        undoHistory_.push([this, materialName, oldSnapshot] { applyMaterialSnapshot(materialName, oldSnapshot); },
                          [this, materialName, newSnapshot] { applyMaterialSnapshot(materialName, newSnapshot); });
    }

    void applyMaterialSnapshot(const std::string& materialName, const nlohmann::json& snapshot) const {
        if (Material* mat = assetManager_.get<Material>(materialName)) {
            if (snapshot.contains("shaderName")) mat->shaderName_ = snapshot["shaderName"].get<std::string>();
            if (snapshot.contains("tint")) mat->tint_ = JsonUtils::getOptional<glm::vec4>(snapshot, "tint", mat->tint_);
            if (snapshot.contains("albedoTexture")) mat->albedoTexture_ = snapshot["albedoTexture"].get<std::string>();
            if (snapshot.contains("normalTexture")) mat->normalTexture_ = snapshot["normalTexture"].get<std::string>();
            if (snapshot.contains("roughnessTexture"))
                mat->roughnessTexture_ = snapshot["roughnessTexture"].get<std::string>();
            if (snapshot.contains("metallicTexture"))
                mat->metallicTexture_ = snapshot["metallicTexture"].get<std::string>();
            if (snapshot.contains("aoTexture")) mat->aoTexture_ = snapshot["aoTexture"].get<std::string>();
            if (snapshot.contains("metallic")) mat->metallic_ = snapshot["metallic"].get<float>();
            if (snapshot.contains("roughness")) mat->roughness_ = snapshot["roughness"].get<float>();
            if (snapshot.contains("ao")) mat->ao_ = snapshot["ao"].get<float>();
            if (snapshot.contains("tiling"))
                mat->tiling_ = JsonUtils::getOptional<glm::vec2>(snapshot, "tiling", mat->tiling_);
            if (snapshot.contains("offset"))
                mat->offset_ = JsonUtils::getOptional<glm::vec2>(snapshot, "offset", mat->offset_);
            if (snapshot.contains("scaleInvariantUV")) mat->scaleInvariantUV_ = snapshot["scaleInvariantUV"].get<int>();
        }
        dirtyMaterials_[materialName] = snapshot;
    }

    void saveMaterial(const std::string& materialName, const nlohmann::json& snapshot) const {
        std::filesystem::path fullPath = assetManager_.getAbsolutePath(materialName);
        std::ofstream file(fullPath);
        file << snapshot.dump(4);
        file.flush();
        file.close();
    }

    void upsertMaterial(const std::filesystem::path& relativePath, const nlohmann::json& snapshot) const {
        std::filesystem::path fullPath = assetManager_.getAbsolutePath(relativePath.string());
        std::ofstream file(fullPath);
        file << snapshot.dump(4);
        file.flush();
        file.close();
        assetManager_.registerAsset(relativePath.string());
    }

    void deleteMaterial_Command(const std::filesystem::path& relativePath) const {
        assetManager_.unload<Material>(relativePath.string());
        std::filesystem::path fullPath = assetManager_.getAbsolutePath(relativePath.string());
        std::filesystem::remove(fullPath);
    }

    void clearSelectionIfSelected(const std::string& materialName) const {
        auto selected = objectSelection_.getSelectedAssetId();
        if (selected && *selected == materialName) {
            objectSelection_.clearSelection();
        }
    }
};
