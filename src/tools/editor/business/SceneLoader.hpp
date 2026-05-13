#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include "../../../engine/modules/scene/EntityManager.hpp"

class GameEngine;
class Scene;
class ObjectSelection;
class EditorAssetRepository;

class SceneLoader {
public:
    using OnSceneNameChanged = std::function<void(const std::string&)>;
    using OnScenePathChanged = std::function<void(const std::filesystem::path&)>;

    SceneLoader(EntityManager& entityManager,
                ObjectSelection& objectSelection,
                GameEngine& engine,
                EditorAssetRepository& assetRepository) :
        entityManager_(entityManager),
        objectSelection_(objectSelection),
        engine_(engine),
        assetRepository_(assetRepository),
        currentScenePath_("") {}

    void newScene();
    void saveScene(const char* path);
    void saveCurrentScene();
    void loadScene(const char* path);
    bool loadLatestScene(const std::filesystem::path& projectPath);

    void setOnSceneNameChanged(OnSceneNameChanged callback) { onSceneNameChanged_ = callback; }

    void setOnScenePathChanged(OnScenePathChanged callback) { onScenePathChanged_ = callback; }

    std::string getCurrentScenePath() const { return currentScenePath_; }

    std::optional<std::string> getSceneName() const { return sceneName_; }

    std::optional<std::filesystem::path> getScenePath() const { return scenePath_; }

private:
    EntityManager& entityManager_;
    ObjectSelection& objectSelection_;
    GameEngine& engine_;
    EditorAssetRepository& assetRepository_;
    OnSceneNameChanged onSceneNameChanged_;
    OnScenePathChanged onScenePathChanged_;
    std::string currentScenePath_;
    std::optional<std::string> sceneName_;
    std::optional<std::filesystem::path> scenePath_;
};
