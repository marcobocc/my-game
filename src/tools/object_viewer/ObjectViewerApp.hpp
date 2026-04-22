#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include "InspectorSidePanel.hpp"
#include "core/GameEngine.hpp"
#include "core/objects/components/Transform.hpp"

class ObjectViewerApp {
public:
    static constexpr float SCENE_WIDTH_RATIO = 0.70f;

    ObjectViewerApp(unsigned int windowWidth, unsigned int windowHeight, const std::filesystem::path& assetsPath) :
        window_(windowWidth, windowHeight, "Object Viewer"),
        engine_(window_, assetsPath),
        scene_(engine_.getScene()) {
        auto [w, h] = window_.getSize();
        window_.setSceneViewport({0, 0, static_cast<int>(static_cast<float>(w) * SCENE_WIDTH_RATIO), h});
        window_.onFramebufferResize([this](int newW, int newH, int, int) {
            window_.setSceneViewport({0, 0, static_cast<int>(static_cast<float>(newW) * SCENE_WIDTH_RATIO), newH});
        });

        setupScene();
        engine_.getUserInterface().emplace<InspectorSidePanel>(&objectId_, &scene_, SCENE_WIDTH_RATIO);
    }

    void run() {
        engine_.run([this](double deltaTime) {
            handlePlayerInput();
            update(deltaTime);
        });
    }

private:
    GameWindow window_;
    GameEngine engine_;
    Scene& scene_;
    std::string objectId_{};

    void setupScene() {
        scene_.createCamera({.position = glm::vec3(0.0f, 0.0f, 4.0f)});
        objectId_ = scene_.createCube({});
    }

    void update(double deltaTime) const {
        // Sample loop: object rotating on itself
        static float totalTime = 0.0f;
        totalTime += static_cast<float>(deltaTime);

        auto& transform = engine_.getScene().getObject(objectId_).add<Transform>();
        transform.rotation = glm::angleAxis(totalTime * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::angleAxis(totalTime * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                             glm::angleAxis(totalTime * 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void handlePlayerInput() const {
        auto& playerInput = engine_.getInputSystem();
        if (playerInput.isKeyPressed(GLFW_KEY_ESCAPE)) engine_.requestClose();
    }
};
