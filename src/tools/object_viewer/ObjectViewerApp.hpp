#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "GameEngineWiringContainer.hpp"
#include "InspectorSidePanel.hpp"
#include "data/components/Transform.hpp"
#include "data/settings/RendererSettings.hpp"

class ObjectViewerApp {
public:
    static constexpr float SCENE_WIDTH_RATIO = 0.70f;
    static constexpr float MOUSE_SENSITIVITY = 0.005f;

    ObjectViewerApp(const std::filesystem::path& assetsPath,
                    unsigned int windowWidth = 0,
                    unsigned int windowHeight = 0) :
        window_("Object Viewer", windowWidth, windowHeight),
        wiringContainer_(window_, assetsPath),
        engine_(wiringContainer_.gameEngine()) {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport({0, 0, static_cast<int>(static_cast<float>(w) * SCENE_WIDTH_RATIO), h});
        window_.onWindowResize([this](int newW, int newH, int, int) {
            window_.setSceneViewport({0, 0, static_cast<int>(static_cast<float>(newW) * SCENE_WIDTH_RATIO), newH});
        });

        setupScene();
        engine_.emplaceWidget<InspectorSidePanel>(&objectId_, engine_, SCENE_WIDTH_RATIO);
    }

    void run() {
        engine_.run([this](double deltaTime) {
            if (engine_.isKeyDown(GLFW_KEY_ESCAPE)) engine_.requestClose();
            update(deltaTime);
        });
    }

private:
    GameWindow window_;
    RendererSettings rendererSettings_;
    GameEngineWiringContainer wiringContainer_;
    GameEngine& engine_;
    std::string objectId_{};
    glm::quat rotation_{1.0f, 0.0f, 0.0f, 0.0f};
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool wasDragging_ = false;

    Camera editorCamera;
    Transform editorCameraTransform{
            .position = glm::vec3(0.0f, 0.0f, 5.0f), .rotation = glm::quat(glm::vec3(0.0f)), .scale = glm::vec3(1.0f)};

    void setupScene() {
        engine_.setActiveCamera(editorCamera, editorCameraTransform);
        objectId_ = engine_.createCube({});
    }

    void update(double /*deltaTime*/) {
        auto [mouseX, mouseY] = engine_.getMousePosition();

        if (engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && isInsideSceneViewport(mouseX, mouseY)) {
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

        engine_.getObject(objectId_).get<Transform>().rotation = rotation_;
    }

    bool isInsideSceneViewport(double mouseX, double mouseY) const {
        auto [w, h] = window_.getLogicalSize();
        return mouseX >= 0 && mouseX < static_cast<double>(w) * SCENE_WIDTH_RATIO && mouseY >= 0 &&
               mouseY < static_cast<double>(h);
    }
};
