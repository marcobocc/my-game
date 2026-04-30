#pragma once
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "core/GameEngineWiringContainer.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/RendererSettings.hpp"
#include "ui/EditorController.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/InspectorPanel.hpp"

class EditorApp {
public:
    static constexpr float ORBIT_SENSITIVITY = 0.005f;
    static constexpr float PAN_SENSITIVITY = 0.01f;
    static constexpr float ZOOM_SENSITIVITY = 0.5f;
    static constexpr float FLY_SPEED = 5.0f;
    static constexpr float SPRINT_MULTIPLIER = 4.0f;
    static constexpr float INITIAL_ORBIT_DISTANCE = 6.0f;
    static constexpr float INITIAL_ORBIT_PITCH = 0.3f;
    static constexpr float INITIAL_ORBIT_YAW = 0.0f;
    static constexpr float MIN_ORBIT_DISTANCE = 0.1f;
    static constexpr float PITCH_CLAMP_MARGIN = 0.05f;

    EditorApp(unsigned int windowWidth, unsigned int windowHeight, const std::filesystem::path& assetsPath) :
        window_(windowWidth, windowHeight, "Editor"),
        wiringContainer_(window_, assetsPath),
        engine_(wiringContainer_.gameEngine()),
        controller_(engine_) {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport(computeSceneViewport(w, h));
        window_.onWindowResize(
                [this](int newW, int newH, int, int) { window_.setSceneViewport(computeSceneViewport(newW, newH)); });
        setupScene();
    }

    void run() {
        engine_.run([this](double deltaTime) { update(deltaTime); });
    }

private:
    GameWindow window_;
    RendererSettings rendererSettings_;
    GameEngineWiringContainer wiringContainer_;
    GameEngine& engine_;
    EditorController controller_;

    glm::vec3 orbitTarget_{0.0f, 0.0f, 0.0f};
    float orbitDistance_ = INITIAL_ORBIT_DISTANCE;
    float orbitYaw_ = 0.0f;
    float orbitPitch_ = INITIAL_ORBIT_PITCH;

    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool wasOrbiting_ = false;
    bool wasPanning_ = false;
    bool wasLeftDown_ = false;

    static SceneViewport computeSceneViewport(int w, int h) {
        int left = static_cast<int>(static_cast<float>(w) * HierarchyPanel::PANEL_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - InspectorPanel::PANEL_WIDTH_RATIO));
        return {left, 0, right - left, h};
    }

    void setupScene() {
        controller_.cameraId = engine_.createCamera({.position = computeCameraPosition()});
        engine_.createCube({});
        engine_.enableWorldGrid();
    }

    glm::vec3 computeCameraPosition() const {
        return {
                orbitTarget_.x + orbitDistance_ * std::cos(orbitPitch_) * std::sin(orbitYaw_),
                orbitTarget_.y + orbitDistance_ * std::sin(orbitPitch_),
                orbitTarget_.z + orbitDistance_ * std::cos(orbitPitch_) * std::cos(orbitYaw_),
        };
    }

    void applyCameraTransform() {
        auto& t = engine_.getObject(controller_.cameraId).get<Transform>();
        t.position = computeCameraPosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - t.position);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::cross(right, forward);
        t.rotation = glm::normalize(glm::quat_cast(glm::mat3(right, up, -forward)));
    }

    void resetOrbitFromCamera() {
        glm::vec3 pos = engine_.getObject(controller_.cameraId).get<Transform>().position;
        orbitTarget_ = {0.0f, 0.0f, 0.0f};
        orbitDistance_ = glm::length(pos);
        orbitPitch_ = std::asin(pos.y / orbitDistance_);
        orbitYaw_ = std::atan2(pos.x, pos.z);
    }

    void update(double deltaTime) {
        auto [mouseX, mouseY] = engine_.getMousePosition();

        bool ctrlHeld = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_S) && controller_.scenePath)
            controller_.saveScene(*controller_.scenePath);

        if (engine_.getObjects().find(controller_.cameraId) == engine_.getObjects().end()) {
            controller_.cameraId = engine_.createCamera({.position = computeCameraPosition()});
        } else if (controller_.scenePath) {
            static std::optional<std::filesystem::path> lastSyncedPath{};
            if (lastSyncedPath != controller_.scenePath) {
                resetOrbitFromCamera();
                lastSyncedPath = controller_.scenePath;
            }
        }

        bool rightDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
        bool middleDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE);

        if (rightDown) {
            if (wasOrbiting_) {
                float dx = static_cast<float>(mouseX - lastMouseX_);
                float dy = static_cast<float>(mouseY - lastMouseY_);
                orbitYaw_ -= dx * ORBIT_SENSITIVITY;
                orbitPitch_ = glm::clamp(orbitPitch_ + dy * ORBIT_SENSITIVITY,
                                         -glm::half_pi<float>() + PITCH_CLAMP_MARGIN,
                                         glm::half_pi<float>() - PITCH_CLAMP_MARGIN);
            }
            wasOrbiting_ = true;
        } else {
            wasOrbiting_ = false;
        }

        if (middleDown) {
            if (wasPanning_) {
                float dx = static_cast<float>(mouseX - lastMouseX_);
                float dy = static_cast<float>(mouseY - lastMouseY_);
                glm::vec3 camPos = computeCameraPosition();
                glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
                glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
                glm::vec3 up = glm::cross(right, forward);
                orbitTarget_ -= right * (dx * PAN_SENSITIVITY);
                orbitTarget_ += up * (dy * PAN_SENSITIVITY);
            }
            wasPanning_ = true;
        } else {
            wasPanning_ = false;
        }

        double scroll = engine_.getScrollDelta();
        if (scroll != 0.0)
            orbitDistance_ =
                    std::max(MIN_ORBIT_DISTANCE, orbitDistance_ - static_cast<float>(scroll) * ZOOM_SENSITIVITY);

        glm::vec3 camPos = computeCameraPosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 move{0.0f};
        if (engine_.isKeyDown(GLFW_KEY_W)) move += forward;
        if (engine_.isKeyDown(GLFW_KEY_S)) move -= forward;
        if (engine_.isKeyDown(GLFW_KEY_A)) move -= right;
        if (engine_.isKeyDown(GLFW_KEY_D)) move += right;
        if (engine_.isKeyPressed(GLFW_KEY_G)) engine_.toggleWorldGrid();
        if (engine_.isKeyPressed(GLFW_KEY_L)) engine_.toggleLighting();
        if (engine_.isKeyPressed(GLFW_KEY_F)) {
            orbitTarget_ = {0.0f, 0.0f, 0.0f};
            orbitDistance_ = INITIAL_ORBIT_DISTANCE;
            orbitYaw_ = INITIAL_ORBIT_YAW;
            orbitPitch_ = INITIAL_ORBIT_PITCH;
        }

        if (glm::length(move) > 0.0f) {
            float speed = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? FLY_SPEED * SPRINT_MULTIPLIER : FLY_SPEED;
            orbitTarget_ += glm::normalize(move) * speed * static_cast<float>(deltaTime);
        }

        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;

        bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        if (leftDown && !wasLeftDown_) {
            auto [scaleX, scaleY] = window_.getContentScale();
            engine_.requestPick(static_cast<uint32_t>(mouseX * scaleX), static_cast<uint32_t>(mouseY * scaleY));
        }
        wasLeftDown_ = leftDown;

        if (auto picked = engine_.getPickResult())
            controller_.selectedObjectId = (controller_.selectedObjectId == picked) ? std::nullopt : std::move(picked);

        if (controller_.selectedObjectId) {
            auto& obj = engine_.getObject(*controller_.selectedObjectId);
            if (obj.has<Renderer>() && obj.has<Transform>())
                engine_.drawObjectOutline(obj.get<Renderer>(), obj.get<Transform>(), *controller_.selectedObjectId);
        }

        applyCameraTransform();
    }
};
