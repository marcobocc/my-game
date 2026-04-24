#pragma once
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "core/GameEngine.hpp"
#include "core/objects/components/Transform.hpp"

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
        scene_(engine_.getScene()) {
        window_.onWindowResize([this](int newW, int newH, int, int) { window_.setSceneViewport({0, 0, newW, newH}); });
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
    Scene& scene_;
    std::string cameraId_{};

    glm::vec3 orbitTarget_{0.0f, 0.0f, 0.0f};
    float orbitDistance_ = INITIAL_ORBIT_DISTANCE;
    float orbitYaw_ = 0.0f;
    float orbitPitch_ = INITIAL_ORBIT_PITCH;

    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool wasOrbiting_ = false;
    bool wasPanning_ = false;

    void setupScene() {
        cameraId_ = scene_.createCamera({.position = computeCameraPosition()});
        scene_.createCube({});
    }

    glm::vec3 computeCameraPosition() const {
        return {
                orbitTarget_.x + orbitDistance_ * std::cos(orbitPitch_) * std::sin(orbitYaw_),
                orbitTarget_.y + orbitDistance_ * std::sin(orbitPitch_),
                orbitTarget_.z + orbitDistance_ * std::cos(orbitPitch_) * std::cos(orbitYaw_),
        };
    }

    void applyCameraTransform() {
        auto& t = scene_.getObject(cameraId_).get<Transform>();
        t.position = computeCameraPosition();

        glm::vec3 forward = glm::normalize(orbitTarget_ - t.position);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::cross(right, forward);
        t.rotation = glm::normalize(glm::quat_cast(glm::mat3(right, up, -forward)));
    }

    void update(double deltaTime) {
        auto& input = engine_.getInputSystem();
        auto [mouseX, mouseY] = input.getMousePosition();

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
        if (scroll != 0.0) {
            orbitDistance_ =
                    std::max(MIN_ORBIT_DISTANCE, orbitDistance_ - static_cast<float>(scroll) * ZOOM_SENSITIVITY);
        }

        // WASD translation: move both camera position and orbit target together.
        glm::vec3 camPos = computeCameraPosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 move{0.0f};
        if (input.isKeyDown(GLFW_KEY_W)) move += forward;
        if (input.isKeyDown(GLFW_KEY_S)) move -= forward;
        if (input.isKeyDown(GLFW_KEY_A)) move -= right;
        if (input.isKeyDown(GLFW_KEY_D)) move += right;
        if (input.isKeyPressed(GLFW_KEY_F)) {
            orbitTarget_ = {0.0f, 0.0f, 0.0f};
            orbitDistance_ = INITIAL_ORBIT_DISTANCE;
            orbitYaw_ = INITIAL_ORBIT_YAW;
            orbitPitch_ = INITIAL_ORBIT_PITCH;
        }

        if (glm::length(move) > 0.0f) {
            float speed = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? FLY_SPEED * SPRINT_MULTIPLIER : FLY_SPEED;
            glm::vec3 delta = glm::normalize(move) * speed * static_cast<float>(deltaTime);
            orbitTarget_ += delta;
        }

        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;

        applyCameraTransform();
    }
};
