#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include "../../../engine/modules/scene/EntityManager.hpp"

class GameEngine;
class Scene;
class ObjectSelection;
class MaterialMutations;

class SceneLoader {
public:
    using OnSceneNameChanged = std::function<void(const std::string&)>;
    using OnScenePathChanged = std::function<void(const std::filesystem::path&)>;

    SceneLoader(EntityManager& entityManager,
                ObjectSelection& objectSelection,
                GameEngine& engine,
                MaterialMutations* materialMutations = nullptr) :
        entityManager_(entityManager),
        objectSelection_(objectSelection),
        engine_(engine),
        materialMutations_(materialMutations) {}

    void newScene();
    void saveScene(const char* path);
    void loadScene(const char* path);
    bool loadLatestScene(const std::filesystem::path& projectPath);

    void setOnSceneNameChanged(OnSceneNameChanged callback) { onSceneNameChanged_ = callback; }

    void setOnScenePathChanged(OnScenePathChanged callback) { onScenePathChanged_ = callback; }

private:
    EntityManager& entityManager_;
    ObjectSelection& objectSelection_;
    GameEngine& engine_;
    MaterialMutations* materialMutations_;
    OnSceneNameChanged onSceneNameChanged_;
    OnScenePathChanged onScenePathChanged_;
};
