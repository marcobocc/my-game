#pragma once
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "../ObjectSelection.hpp"
#include "../scene_editing/UndoHistory.hpp"
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
        Material defaultMaterial(materialName, GBUFFER_SHADER, glm::vec4(1.0f), EMPTY_TEXTURE);
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

    void setBaseColor(const std::string& materialName, const glm::vec4& color) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["baseColor"] = JsonUtils::serializeVec4(color);
            updateMaterial(materialName, oldSnapshot, newSnapshot);
        }
    }

    void setTexture(const std::string& materialName, const std::string& textureName) const {
        if (isBuiltin(materialName)) return;
        if (const Material* mat = assetManager_.get<Material>(materialName)) {
            nlohmann::json oldSnapshot = serializeMaterial(*mat);
            nlohmann::json newSnapshot = oldSnapshot;
            newSnapshot["textureName"] = textureName;
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

    bool isBuiltin(const std::string& materialName) const { return assetManager_.isBuiltin(materialName); }

private:
    AssetManager& assetManager_;
    ObjectSelection& objectSelection_;
    UndoHistory& undoHistory_;

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
        j["baseColor"] = JsonUtils::serializeVec4(material.getBaseColor());
        j["textureName"] = material.getTextureName();
        return j;
    }

    void updateMaterial(const std::string& materialName,
                        const nlohmann::json& oldSnapshot,
                        const nlohmann::json& newSnapshot) const {
        saveMaterial(materialName, newSnapshot);
        assetManager_.reload<Material>(materialName);
        undoHistory_.push(
                [this, materialName, oldSnapshot, newSnapshot] {
                    updateMaterial(materialName, newSnapshot, oldSnapshot);
                },
                [this, materialName, oldSnapshot, newSnapshot] {
                    updateMaterial(materialName, oldSnapshot, newSnapshot);
                });
    }

    void saveMaterial(const std::string& materialName, const nlohmann::json& snapshot) const {
        std::filesystem::path fullPath = assetManager_.getAbsolutePath(materialName);
        std::ofstream file(fullPath);
        file << snapshot.dump(4);
        file.close();
    }

    void upsertMaterial(const std::filesystem::path& relativePath, const nlohmann::json& snapshot) const {
        std::filesystem::path fullPath = assetManager_.getAbsolutePath(relativePath.string());
        std::ofstream file(fullPath);
        file << snapshot.dump(4);
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
