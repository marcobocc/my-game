#include "EditorCamera.hpp"
#include <cmath>
#include <glm/gtc/constants.hpp>

EditorCamera::EditorCamera() { resetToDefault(); }

void EditorCamera::resetToDefault() {
    transform_.position = {0.0f, 2.0f, INITIAL_DISTANCE};
    yaw_ = glm::pi<float>();
    pitch_ = 0.3f;
    applyRotation();
}

void EditorCamera::resetFromTransform(const Transform& t) {
    transform_.position = t.position;
    transform_.rotation = t.rotation;
    glm::vec3 forward = transform_.getForward();
    pitch_ = std::asin(forward.y);
    yaw_ = std::atan2(-forward.x, -forward.z);
}

void EditorCamera::orbit(float deltaX, float deltaY) {
    yaw_ -= deltaX * ORBIT_SENSITIVITY;
    pitch_ = glm::clamp(pitch_ + deltaY * ORBIT_SENSITIVITY,
                        -glm::half_pi<float>() + PITCH_CLAMP_MARGIN,
                        glm::half_pi<float>() - PITCH_CLAMP_MARGIN);
    applyRotation();
}

void EditorCamera::pan(float deltaX, float deltaY) {
    glm::vec3 right = transform_.getRight();
    glm::vec3 up = transform_.getUp();
    transform_.position -= right * (deltaX * PAN_SENSITIVITY);
    transform_.position += up * (deltaY * PAN_SENSITIVITY);
}

void EditorCamera::zoom(double scrollDelta) {
    if (scrollDelta != 0.0) {
        transform_.position += transform_.getForward() * static_cast<float>(scrollDelta) * ZOOM_SENSITIVITY;
    }
}

void EditorCamera::moveTarget(const glm::vec3& direction, float speed, double deltaTime) {
    if (glm::length(direction) > 0.0f) {
        transform_.position += glm::normalize(direction) * speed * static_cast<float>(deltaTime);
    }
}

void EditorCamera::moveFromKeyboard(uint8_t direction, bool sprint, double deltaTime) {
    glm::vec3 forward = transform_.getForward();
    glm::vec3 right = transform_.getRight();

    glm::vec3 move{0.0f};
    if (direction & 1) move += forward; // W
    if (direction & 2) move -= forward; // S
    if (direction & 4) move += right; // A
    if (direction & 8) move -= right; // D

    float speed = sprint ? FLY_SPEED * SPRINT_MULTIPLIER : FLY_SPEED;
    moveTarget(move, speed, deltaTime);
}

void EditorCamera::applyRotation() {
    glm::vec3 eulerAngles(pitch_, yaw_, 0.0f);
    transform_.rotation = glm::normalize(glm::quat(eulerAngles));
}
