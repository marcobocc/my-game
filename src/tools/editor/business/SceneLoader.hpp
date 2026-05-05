#pragma once
#include <functional>
#include <string>

class GameEngine;
class Scene;
class ObjectSelection;

class SceneLoader {
public:
    using OnSceneNameChanged = std::function<void(const std::string&)>;

    SceneLoader(Scene& sceneManager, ObjectSelection& objectSelection, GameEngine& engine) :
        scene_(sceneManager),
        objectSelection_(objectSelection),
        engine_(engine) {}

    void newScene();
    void saveScene(const char* path);
    void loadScene(const char* path);

private:
    Scene& scene_;
    ObjectSelection& objectSelection_;
    GameEngine& engine_;
    OnSceneNameChanged onSceneNameChanged_;
};
