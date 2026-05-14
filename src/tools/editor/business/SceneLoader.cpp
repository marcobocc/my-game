#include "SceneLoader.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/utils/JsonUtils.hpp"
#include "ObjectSelection.hpp"
#include "asset_editing/EditorAssetRepository.hpp"
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

void SceneLoader::newScene() {
    entityManager_.clear();
    glm::quat rotation = glm::angleAxis(glm::radians(135.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
    Transform t{glm::vec3(0.0f), rotation, glm::vec3(1.0f)};
    Light l{LightType::Directional, 1.0f};
    nlohmann::json lightData{
            {"metadata", {{"name", "Light"}}},
            {"transform", t.serialize()},
            {"light", l.serialize()},
    };
    entityManager_.upsertFromJson(lightData);
}

void SceneLoader::saveScene(const char* path) {
    // Flush any dirty materials to disk before saving the scene
    assetRepository_.commit<Material>();

    std::ofstream f(path);
    if (!f) return;
    nlohmann::json root;
    nlohmann::json entities = nlohmann::json::array();
    for (const EntityHandle& entity: entityManager_.getEntities())
        entities.push_back(entityManager_.serializeToJson(entity));
    root["entities"] = entities;
    f << root.dump(4);

    currentScenePath_ = path;

    if (onSceneNameChanged_) {
        std::string pathStr(path);
        size_t lastSlash = pathStr.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
        sceneName_ = filename;
        onSceneNameChanged_(filename);
    }
    if (onScenePathChanged_) {
        scenePath_ = std::filesystem::path(path);
        onScenePathChanged_(std::filesystem::path(path));
    }
}

void SceneLoader::saveCurrentScene() {
    if (!currentScenePath_.empty()) {
        saveScene(currentScenePath_.c_str());
    }
}

void SceneLoader::loadScene(const char* path) {
    try {
        auto json = JsonUtils::loadJson(path);
        entityManager_.clear();
        for (const auto& entityJson: json.at("entities"))
            entityManager_.upsertFromJson(entityJson);
        objectSelection_.clearSelection();

        currentScenePath_ = path;

        if (onSceneNameChanged_) {
            std::string pathStr(path);
            size_t lastSlash = pathStr.find_last_of("/\\");
            std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
            sceneName_ = filename;
            onSceneNameChanged_(filename);
        }
        if (onScenePathChanged_) {
            scenePath_ = std::filesystem::path(path);
            onScenePathChanged_(std::filesystem::path(path));
        }
    } catch (const std::exception&) {
        // Scene loading failed
    }
}

bool SceneLoader::loadLatestScene() {
    std::string latestScene;
    int64_t latestTime = -1;

    for (const auto& file: vfs_.listFilesRecursive()) {
        if (std::filesystem::path(file).extension() == ".scene") {
            auto modTime = vfs_.getModifiedTime(file);
            if (latestScene.empty() || modTime > latestTime) {
                latestScene = file;
                latestTime = modTime;
            }
        }
    }

    if (latestScene.empty()) return false;

    try {
        auto data = vfs_.read(latestScene);
        auto json = nlohmann::json::parse(data.begin(), data.end());
        entityManager_.clear();
        for (const auto& entityJson: json.at("entities"))
            entityManager_.upsertFromJson(entityJson);
        objectSelection_.clearSelection();
        currentScenePath_ = latestScene;
        if (onSceneNameChanged_) {
            std::string filename = std::filesystem::path(latestScene).filename().string();
            sceneName_ = filename;
            onSceneNameChanged_(filename);
        }
        if (onScenePathChanged_) {
            scenePath_ = std::filesystem::path(latestScene);
            onScenePathChanged_(scenePath_.value());
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}
