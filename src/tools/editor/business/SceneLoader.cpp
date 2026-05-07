#include "SceneLoader.hpp"
#include <fstream>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/modules/scene/Scene.hpp"
#include "../../../engine/utils/JsonUtils.hpp"
#include "ObjectSelection.hpp"

void SceneLoader::newScene() { engine_.createCube({}); }

void SceneLoader::saveScene(const char* path) {
    std::ofstream f(path);
    if (!f) return;
    f << scene_.serialize().dump(4);
    if (onSceneNameChanged_) {
        std::string pathStr(path);
        size_t lastSlash = pathStr.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
        onSceneNameChanged_(filename);
    }
}

void SceneLoader::loadScene(const char* path) {
    try {
        Scene::deserialize(JsonUtils::loadJson(path), scene_);
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
