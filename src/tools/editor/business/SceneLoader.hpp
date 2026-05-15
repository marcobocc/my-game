#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include "../../../engine/modules/asset_management/VirtualFileSystem.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"

class GameEngine;
class Scene;
class EditorSelection;
class EditorAssetRepository;

class SceneLoader {
public:
    using OnSceneNameChanged = std::function<void(const std::string&)>;
    using OnScenePathChanged = std::function<void(const std::filesystem::path&)>;

    SceneLoader(EntityManager& entityManager,
                EditorSelection& editorSelection,
                GameEngine& engine,
                EditorAssetRepository& assetRepository,
                VirtualFileSystem& vfs) :
        entityManager_(entityManager),
        editorSelection_(editorSelection),
        engine_(engine),
        assetRepository_(assetRepository),
        vfs_(vfs),
        currentScenePath_("") {}

    void newScene();
    void saveScene(const char* path);
    void saveCurrentScene();
    void loadScene(const char* path);
    bool loadLatestScene();

    void setOnSceneNameChanged(OnSceneNameChanged callback) { onSceneNameChanged_ = callback; }

    void setOnScenePathChanged(OnScenePathChanged callback) { onScenePathChanged_ = callback; }

    std::string getCurrentScenePath() const { return currentScenePath_; }

    std::optional<std::string> getSceneName() const { return sceneName_; }

    std::optional<std::filesystem::path> getScenePath() const { return scenePath_; }

private:
    EntityManager& entityManager_;
    EditorSelection& editorSelection_;
    GameEngine& engine_;
    EditorAssetRepository& assetRepository_;
    VirtualFileSystem& vfs_;
    OnSceneNameChanged onSceneNameChanged_;
    OnScenePathChanged onScenePathChanged_;
    std::string currentScenePath_;
    std::optional<std::string> sceneName_;
    std::optional<std::filesystem::path> scenePath_;
};
