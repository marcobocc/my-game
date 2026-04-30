#pragma once
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include "GameEngine.hpp"
#include "data/components/Transform.hpp"

class OrbitCameraController {
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

    explicit OrbitCameraController(GameEngine& engine) : engine_(engine) {}

    glm::vec3 computePosition() const {
        return {
                orbitTarget_.x + orbitDistance_ * std::cos(orbitPitch_) * std::sin(orbitYaw_),
                orbitTarget_.y + orbitDistance_ * std::sin(orbitPitch_),
                orbitTarget_.z + orbitDistance_ * std::cos(orbitPitch_) * std::cos(orbitYaw_),
        };
    }

    void resetFromCamera(const std::string& cameraId) {
        glm::vec3 pos = engine_.getObject(cameraId).get<Transform>().position;
        orbitTarget_ = {0.0f, 0.0f, 0.0f};
        orbitDistance_ = glm::length(pos);
        orbitPitch_ = std::asin(pos.y / orbitDistance_);
        orbitYaw_ = std::atan2(pos.x, pos.z);
    }

    void resetToDefault() {
        orbitTarget_ = {0.0f, 0.0f, 0.0f};
        orbitDistance_ = INITIAL_ORBIT_DISTANCE;
        orbitYaw_ = INITIAL_ORBIT_YAW;
        orbitPitch_ = INITIAL_ORBIT_PITCH;
    }

    void update(double deltaTime, const std::string& cameraId) {
        auto [mouseX, mouseY] = engine_.getMousePosition();

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
                glm::vec3 camPos = computePosition();
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

        glm::vec3 camPos = computePosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 move{0.0f};
        if (engine_.isKeyDown(GLFW_KEY_W)) move += forward;
        if (engine_.isKeyDown(GLFW_KEY_S)) move -= forward;
        if (engine_.isKeyDown(GLFW_KEY_A)) move -= right;
        if (engine_.isKeyDown(GLFW_KEY_D)) move += right;
        if (engine_.isKeyPressed(GLFW_KEY_F)) resetToDefault();

        if (glm::length(move) > 0.0f) {
            float speed = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) ? FLY_SPEED * SPRINT_MULTIPLIER : FLY_SPEED;
            orbitTarget_ += glm::normalize(move) * speed * static_cast<float>(deltaTime);
        }

        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;

        applyTransform(cameraId);
    }

private:
    GameEngine& engine_;

    glm::vec3 orbitTarget_{0.0f, 0.0f, 0.0f};
    float orbitDistance_ = INITIAL_ORBIT_DISTANCE;
    float orbitYaw_ = INITIAL_ORBIT_YAW;
    float orbitPitch_ = INITIAL_ORBIT_PITCH;

    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool wasOrbiting_ = false;
    bool wasPanning_ = false;

    void applyTransform(const std::string& cameraId) {
        auto& t = engine_.getObject(cameraId).get<Transform>();
        t.position = computePosition();
        glm::vec3 forward = glm::normalize(orbitTarget_ - t.position);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::cross(right, forward);
        t.rotation = glm::normalize(glm::quat_cast(glm::mat3(right, up, -forward)));
    }
};
