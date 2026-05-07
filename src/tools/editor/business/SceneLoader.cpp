#include "SceneLoader.hpp"
#include <fstream>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/utils/JsonUtils.hpp"
#include "ObjectSelection.hpp"

void SceneLoader::newScene() { entityManager_.clear(); }

void SceneLoader::saveScene(const char* path) {
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
            entityManager_.createFromJson(entityJson);
        objectSelection_.clearSelection();
        if (onSceneNameChanged_) {
            std::string pathStr(path);
            size_t lastSlash = pathStr.find_last_of("/\\");
            std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
            onSceneNameChanged_(filename);
        }
    } catch (const std::exception&) {
        // Scene loading failed
    }
}
