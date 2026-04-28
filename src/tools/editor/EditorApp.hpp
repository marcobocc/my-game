#pragma once
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "core/GameEngine.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"
#include "ui/EditorController.hpp"
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
        engine_(window_, assetsPath),
        controller_(engine_) {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport({0, 0, static_cast<int>(w * (1.0f - InspectorPanel::PANEL_WIDTH_RATIO)), h});
        window_.onWindowResize([this](int newW, int newH, int, int) {
            window_.setSceneViewport({0, 0, static_cast<int>(newW * (1.0f - InspectorPanel::PANEL_WIDTH_RATIO)), newH});
        });
        setupScene();
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

    void setupScene() {
        controller_.cameraId = engine_.getScene().createCamera({.position = computeCameraPosition()});
        engine_.getScene().createCube({});
        engine_.getRendererSettings().enableGrid = true;
    }

    glm::vec3 computeCameraPosition() const {
        return {
                orbitTarget_.x + orbitDistance_ * std::cos(orbitPitch_) * std::sin(orbitYaw_),
                orbitTarget_.y + orbitDistance_ * std::sin(orbitPitch_),
                orbitTarget_.z + orbitDistance_ * std::cos(orbitPitch_) * std::cos(orbitYaw_),
        };
    }

    void applyCameraTransform() {
        auto& t = engine_.getScene().getObject(controller_.cameraId).get<Transform>();
        t.position = computeCameraPosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - t.position);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::cross(right, forward);
        t.rotation = glm::normalize(glm::quat_cast(glm::mat3(right, up, -forward)));
    }

    void resetOrbitFromCamera() {
        glm::vec3 pos = engine_.getScene().getObject(controller_.cameraId).get<Transform>().position;
        orbitTarget_ = {0.0f, 0.0f, 0.0f};
        orbitDistance_ = glm::length(pos);
        orbitPitch_ = std::asin(pos.y / orbitDistance_);
        orbitYaw_ = std::atan2(pos.x, pos.z);
    }

    void update(double deltaTime) {
        auto& input = engine_.getInputSystem();
        auto& scene = engine_.getScene();
        auto [mouseX, mouseY] = input.getMousePosition();

        bool ctrlHeld = input.isKeyDown(GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        if (ctrlHeld && input.isKeyPressed(GLFW_KEY_S) && controller_.scenePath)
            controller_.saveScene(*controller_.scenePath);

        if (scene.getObjects().find(controller_.cameraId) == scene.getObjects().end()) {
            controller_.cameraId = scene.createCamera({.position = computeCameraPosition()});
        } else if (controller_.scenePath) {
            static std::optional<std::filesystem::path> lastSyncedPath{};
            if (lastSyncedPath != controller_.scenePath) {
                resetOrbitFromCamera();
                lastSyncedPath = controller_.scenePath;
            }
        }

        bool rightDown = input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
        bool middleDown = input.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE);

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

        double scroll = input.getScrollDelta();
        if (scroll != 0.0)
            orbitDistance_ =
                    std::max(MIN_ORBIT_DISTANCE, orbitDistance_ - static_cast<float>(scroll) * ZOOM_SENSITIVITY);

        glm::vec3 camPos = computeCameraPosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 move{0.0f};
        if (input.isKeyDown(GLFW_KEY_W)) move += forward;
        if (input.isKeyDown(GLFW_KEY_S)) move -= forward;
        if (input.isKeyDown(GLFW_KEY_A)) move -= right;
        if (input.isKeyDown(GLFW_KEY_D)) move += right;
        if (input.isKeyPressed(GLFW_KEY_G))
            engine_.getRendererSettings().enableGrid = !engine_.getRendererSettings().enableGrid;
        if (input.isKeyPressed(GLFW_KEY_L))
            engine_.getRendererSettings().enableLighting = !engine_.getRendererSettings().enableLighting;
        if (input.isKeyPressed(GLFW_KEY_F)) {
            orbitTarget_ = {0.0f, 0.0f, 0.0f};
            orbitDistance_ = INITIAL_ORBIT_DISTANCE;
            orbitYaw_ = INITIAL_ORBIT_YAW;
            orbitPitch_ = INITIAL_ORBIT_PITCH;
        }

        if (glm::length(move) > 0.0f) {
            float speed = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? FLY_SPEED * SPRINT_MULTIPLIER : FLY_SPEED;
            orbitTarget_ += glm::normalize(move) * speed * static_cast<float>(deltaTime);
        }

        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;

        bool leftDown = input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        if (leftDown && !wasLeftDown_) {
            auto [scaleX, scaleY] = window_.getContentScale();
            engine_.getPickingSystem().requestPick(static_cast<uint32_t>(mouseX * scaleX),
                                                   static_cast<uint32_t>(mouseY * scaleY));
        }
        wasLeftDown_ = leftDown;

        if (auto picked = engine_.getPickingSystem().getPickResult())
            controller_.selectedObjectId = (controller_.selectedObjectId == picked) ? std::nullopt : std::move(picked);

        if (controller_.selectedObjectId) {
            auto& obj = scene.getObject(*controller_.selectedObjectId);
            if (obj.has<Renderer>() && obj.has<Transform>())
                engine_.outline(obj.get<Renderer>(), obj.get<Transform>(), *controller_.selectedObjectId);
        }

        applyCameraTransform();
    }
};
