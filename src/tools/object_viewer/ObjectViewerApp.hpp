#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "InspectorSidePanel.hpp"
#include "core/GameEngine.hpp"
#include "core/objects/components/Transform.hpp"

class ObjectViewerApp {
public:
    static constexpr float SCENE_WIDTH_RATIO = 0.70f;
    static constexpr float MOUSE_SENSITIVITY = 0.005f;

    ObjectViewerApp(unsigned int windowWidth, unsigned int windowHeight, const std::filesystem::path& assetsPath) :
        window_(windowWidth, windowHeight, "Object Viewer"),
        engine_(window_, assetsPath),
        scene_(engine_.getScene()) {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport({0, 0, static_cast<int>(static_cast<float>(w) * SCENE_WIDTH_RATIO), h});
        window_.onWindowResize([this](int newW, int newH, int, int) {
            window_.setSceneViewport({0, 0, static_cast<int>(static_cast<float>(newW) * SCENE_WIDTH_RATIO), newH});
        });

        setupScene();
        engine_.getUserInterface().emplace<InspectorSidePanel>(
                &objectId_, &scene_, &engine_.getAssetManager(), SCENE_WIDTH_RATIO);
    }

    void run() {
        engine_.run([this](double deltaTime) {
            if (engine_.getInputSystem().isKeyDown(GLFW_KEY_ESCAPE)) engine_.requestClose();
            update(deltaTime);
        });
    }

private:
    GameWindow window_;
    GameEngine engine_;
    Scene& scene_;
    std::string objectId_{};
    glm::quat rotation_{1.0f, 0.0f, 0.0f, 0.0f};
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool wasDragging_ = false;

    void setupScene() {
        scene_.createCamera({.position = glm::vec3(0.0f, 0.0f, 4.0f)});
        objectId_ = scene_.createCube({});
    }

    void update(double /*deltaTime*/) {
        auto& input = engine_.getInputSystem();
        auto [mouseX, mouseY] = input.getMousePosition();

        if (input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && isInsideSceneViewport(mouseX, mouseY)) {
            if (wasDragging_) {
                float dx = static_cast<float>(mouseX - lastMouseX_);
                float dy = static_cast<float>(mouseY - lastMouseY_);
                glm::quat yaw = glm::angleAxis(dx * MOUSE_SENSITIVITY, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::quat pitch = glm::angleAxis(dy * MOUSE_SENSITIVITY, glm::vec3(1.0f, 0.0f, 0.0f));
                rotation_ = glm::normalize(yaw * rotation_ * pitch);
            }
            wasDragging_ = true;
        } else {
            wasDragging_ = false;
        }

        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;

        engine_.getScene().getObject(objectId_).get<Transform>().rotation = rotation_;
    }

    bool isInsideSceneViewport(double mouseX, double mouseY) const {
        auto [w, h] = window_.getLogicalSize();
        return mouseX >= 0 && mouseX < static_cast<double>(w) * SCENE_WIDTH_RATIO && mouseY >= 0 &&
               mouseY < static_cast<double>(h);
    }
};
