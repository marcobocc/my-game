#pragma once
#include <filesystem>
#include <optional>
#include "common_editing/RuntimeScene.hpp"
#include "modules/asset_management/VirtualFileSystem.hpp"
#include "modules/scene/World.hpp"

class EditorSelection;
class AssetStore;
class UndoHistory;
class SceneQuickActions;

class EditorContext {
public:
    EditorContext(World& entityManager,
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
    // Mount project assets, set write dir, and load/create scene. Use when project root was not known at startup.
    void openProject(const std::filesystem::path& projectRoot);
    // Load/create scene for a project root that was already mounted at startup.
    void initProject();

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
