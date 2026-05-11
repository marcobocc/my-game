#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include "../../../engine/modules/scene/EntityManager.hpp"

class GameEngine;
class Scene;
class ObjectSelection;

class SceneLoader {
public:
    using OnSceneNameChanged = std::function<void(const std::string&)>;

    SceneLoader(EntityManager& entityManager, ObjectSelection& objectSelection, GameEngine& engine) :
        entityManager_(entityManager),
        objectSelection_(objectSelection),
        engine_(engine) {}

    void newScene();
    void saveScene(const char* path);
    void loadScene(const char* path);
    bool loadLatestScene(const std::filesystem::path& projectPath);

private:
    EntityManager& entityManager_;
    ObjectSelection& objectSelection_;
    GameEngine& engine_;
    OnSceneNameChanged onSceneNameChanged_;
};
