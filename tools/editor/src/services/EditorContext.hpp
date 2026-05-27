#pragma once
#include <filesystem>
#include <optional>
#include "common_editing/RuntimeScene.hpp"
#include "modules/asset_management/VirtualFileSystem.hpp"
#include "modules/scene/EntityStore.hpp"

class EditorSelection;
class AssetStore;
class UndoHistory;
class SceneQuickActions;

class EditorContext {
public:
    EditorContext(EntityManager& entityManager,
                  EditorSelection& editorSelection,
                  AssetStore& assetStore,
                  VirtualFileSystem& vfs,
                  SceneQuickActions& sceneQuickActions,
                  UndoHistory& undoHistory) :
        editorSelection_(editorSelection),
        assetStore(assetStore),
        vfs_(vfs),
        sceneQuickActions_(sceneQuickActions),
        undoHistory_(undoHistory),
        scene_(entityManager, undoHistory) {}

    void newScene();
    void saveScene(const char* path);
    void saveCurrentScene();
    bool loadScene(const char* path);
    bool loadLatestScene();

    RuntimeScene& getCurrentScene() { return scene_; }
    std::optional<std::filesystem::path> getCurrentScenePath() const { return currentScenePath_; }

private:
    EditorSelection& editorSelection_;
    AssetStore& assetStore;
    VirtualFileSystem& vfs_;
    SceneQuickActions& sceneQuickActions_;
    UndoHistory& undoHistory_;
    RuntimeScene scene_;
    std::optional<std::filesystem::path> currentScenePath_;
};
