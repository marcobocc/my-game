#include "EditorContext.hpp"
#include <fstream>
#include "../../../runtime/utils/JsonUtils.hpp"
#include "EditorSelection.hpp"
#include "UndoHistory.hpp"
#include "common_editing/SceneQuickActions.hpp"
#include "common_editing/transport/SceneDTO.hpp"

void EditorContext::newScene() {
    scene_.clearScene();
    sceneQuickActions_.createLight();
    sceneQuickActions_.createCube();
    undoHistory_.clear();
}

void EditorContext::saveScene(const char* path) {
    assetStore.saveChanges();
    std::ofstream f(path);
    if (!f) return;
    nlohmann::json j = scene_.snapshotScene().serialize();
    f << j.dump(2);
    currentScenePath_ = std::filesystem::path(path);
}

void EditorContext::saveCurrentScene() {
    if (currentScenePath_) saveScene(currentScenePath_->string().c_str());
}

bool EditorContext::loadScene(const char* path) {
    try {
        auto json = JsonUtils::loadJson(path);
        SceneDTO dto = SceneDTO::deserialize(json);
        scene_.restoreScene(dto, "Load Scene");
        editorSelection_.clearSelection();
        currentScenePath_ = std::filesystem::path(path);
        undoHistory_.clear();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool EditorContext::loadLatestScene() {
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
    return loadScene(latestScene.c_str());
}
