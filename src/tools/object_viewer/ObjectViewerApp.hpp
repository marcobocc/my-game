#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include "core/GameEngine.hpp"
#include "core/components/TransformComponent.hpp"
#include "core/utils/SceneBuilder.hpp"

class ObjectViewerApp {
public:
    ObjectViewerApp(unsigned int windowWidth, unsigned int windowHeight) :
        engine_(windowWidth, windowHeight, "Object Viewer") {
        setupScene();
    }

    void run() {
        engine_.run([this](double deltaTime) {
            handlePlayerInput();
            update(deltaTime);
        });
    }

private:
    GameEngine engine_;
    unsigned int objectId_{};

    void setupScene() {
        SceneBuilder sceneBuilder(engine_.getECS(), engine_.getAssetManager());
        sceneBuilder.addCamera(glm::vec3(0.0f, 0.0f, 4.0f));
        objectId_ = sceneBuilder.addCube();
    }

    void update(double deltaTime) const {
        // Sample loop: object rotating on itself
        static float totalTime = 0.0f;
        totalTime += static_cast<float>(deltaTime);

        auto& transform = engine_.getECS().getComponent<TransformComponent>(objectId_);
        transform.rotation = glm::angleAxis(totalTime * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::angleAxis(totalTime * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                             glm::angleAxis(totalTime * 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void handlePlayerInput() const {
        auto& playerInput = engine_.getPlayerInput();
        if (playerInput.keys.at(GLFW_KEY_ESCAPE)) {
            engine_.requestClose();
        }
    }
};
