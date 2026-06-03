#include "EditorContext.hpp"
#include <fstream>
#include "EditorSelection.hpp"
#include "UndoHistory.hpp"
#include "common_editing/SceneQuickActions.hpp"
#include "common_editing/transport/SceneDTO.hpp"
#include "utils/JsonUtils.hpp"

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

void EditorContext::openProject(const std::filesystem::path& projectRoot) {
    vfs_.mount(projectRoot);
    vfs_.setWriteDir(projectRoot);
    if (!loadLatestScene()) newScene();
}

void EditorContext::initProject() {
    if (!loadLatestScene()) newScene();
}

bool EditorContext::loadLatestScene() {
    std::string latestVfsPath;
    int64_t latestTime = -1;
    for (const auto& file: vfs_.listFilesRecursive()) {
        if (std::filesystem::path(file).extension() == ".scene") {
            auto modTime = vfs_.getModifiedTime(file);
            if (latestVfsPath.empty() || modTime > latestTime) {
                latestVfsPath = file;
                latestTime = modTime;
            }
        }
    }
    if (latestVfsPath.empty()) return false;
    auto realPath = vfs_.getRealPath(latestVfsPath);
    if (!realPath) return false;
    return loadScene(realPath->string().c_str());
}
