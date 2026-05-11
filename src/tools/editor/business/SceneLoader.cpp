#include "SceneLoader.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/utils/JsonUtils.hpp"
#include "ObjectSelection.hpp"
#include "asset_editing/EditorAssetRepository.hpp"
#include "modules/asset_management/assets/Material.hpp"
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
    if (onSceneNameChanged_) {
        std::string pathStr(path);
        size_t lastSlash = pathStr.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
        onSceneNameChanged_(filename);
    }
}

void SceneLoader::loadScene(const char* path) {
    try {
        auto json = JsonUtils::loadJson(path);
        entityManager_.clear();
        for (const auto& entityJson: json.at("entities"))
            entityManager_.upsertFromJson(entityJson);
        objectSelection_.clearSelection();
        if (onSceneNameChanged_) {
            std::string pathStr(path);
            size_t lastSlash = pathStr.find_last_of("/\\");
            std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
            onSceneNameChanged_(filename);
        }
        if (onScenePathChanged_) {
            onScenePathChanged_(std::filesystem::path(path));
        }
    } catch (const std::exception&) {
        // Scene loading failed
    }
}

bool SceneLoader::loadLatestScene(const std::filesystem::path& projectPath) {

    if (projectPath.empty() || !std::filesystem::exists(projectPath)) {
        return false;
    }

    std::filesystem::path latestScenePath;
    std::filesystem::file_time_type latestTime;

    try {
        for (const auto& entry: std::filesystem::directory_iterator(projectPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".scene") {
                auto lastWriteTime = std::filesystem::last_write_time(entry);
                if (latestScenePath.empty() || lastWriteTime > latestTime) {
                    latestScenePath = entry.path();
                    latestTime = lastWriteTime;
                }
            }
        }
    } catch (const std::exception& e) {
        return false;
    }

    if (!latestScenePath.empty()) {
        loadScene(latestScenePath.c_str());
        return true;
    }
    return false;
}
