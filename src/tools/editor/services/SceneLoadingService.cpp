#include "SceneLoadingService.hpp"
#include <fstream>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/utils/JsonUtils.hpp"
#include "../state/EditorState.hpp"

void SceneLoadingService::newScene() { engine_.createCube({}); }

void SceneLoadingService::saveScene(const char* path) {
    std::ofstream f(path);
    if (!f) return;
    f << editorState_.sceneManager.serialize().dump(4);
    if (onSceneNameChanged_) {
        std::string pathStr(path);
        size_t lastSlash = pathStr.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
        onSceneNameChanged_(filename);
    }
}

void SceneLoadingService::loadScene(const char* path) {
    try {
        SceneManager::deserialize(JsonUtils::loadJson(path), editorState_.sceneManager);
        editorState_.selectedObjectId = std::nullopt;
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
