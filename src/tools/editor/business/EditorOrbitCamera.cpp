#include "EditorOrbitCamera.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

EditorOrbitCamera::EditorOrbitCamera() { resetToDefault(); }

void EditorOrbitCamera::resetToDefault() {
    orbitTarget_ = {0.0f, 0.0f, 0.0f};
    orbitDistance_ = INITIAL_ORBIT_DISTANCE;
    orbitYaw_ = INITIAL_ORBIT_YAW;
    orbitPitch_ = INITIAL_ORBIT_PITCH;
    applyTransform();
}

void EditorOrbitCamera::resetFromTransform(const Transform& t) {
    orbitTarget_ = {0.0f, 0.0f, 0.0f};
    orbitDistance_ = glm::length(t.position);
    orbitPitch_ = std::asin(t.position.y / orbitDistance_);
    orbitYaw_ = std::atan2(t.position.x, t.position.z);
    applyTransform();
}

void EditorOrbitCamera::orbit(float deltaX, float deltaY) {
    orbitYaw_ -= deltaX * ORBIT_SENSITIVITY;
    orbitPitch_ = glm::clamp(orbitPitch_ + deltaY * ORBIT_SENSITIVITY,
                             -glm::half_pi<float>() + PITCH_CLAMP_MARGIN,
                             glm::half_pi<float>() - PITCH_CLAMP_MARGIN);
    applyTransform();
}

void EditorOrbitCamera::pan(float deltaX, float deltaY) {
    glm::vec3 camPos = computePosition();
    glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::cross(right, forward);
    orbitTarget_ -= right * (deltaX * PAN_SENSITIVITY);
    orbitTarget_ += up * (deltaY * PAN_SENSITIVITY);
    applyTransform();
}

void EditorOrbitCamera::zoom(double scrollDelta) {
    if (scrollDelta != 0.0) {
        orbitDistance_ =
                std::max(MIN_ORBIT_DISTANCE, orbitDistance_ - static_cast<float>(scrollDelta) * ZOOM_SENSITIVITY);
        applyTransform();
    }
}

void EditorOrbitCamera::moveTarget(const glm::vec3& direction, float speed, double deltaTime) {
    if (glm::length(direction) > 0.0f) {
        orbitTarget_ += glm::normalize(direction) * speed * static_cast<float>(deltaTime);
        applyTransform();
    }
}

glm::vec3 EditorOrbitCamera::computePosition() const {
    return {
            orbitTarget_.x + orbitDistance_ * std::cos(orbitPitch_) * std::sin(orbitYaw_),
            orbitTarget_.y + orbitDistance_ * std::sin(orbitPitch_),
            orbitTarget_.z + orbitDistance_ * std::cos(orbitPitch_) * std::cos(orbitYaw_),
    };
}

void EditorOrbitCamera::moveFromKeyboard(uint8_t direction, bool sprint, double deltaTime) {
    glm::vec3 camPos = computePosition();
    glm::vec3 forward = glm::normalize(orbitTarget_ - camPos);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 move{0.0f};
    if (direction & 1) move += forward; // W
    if (direction & 2) move -= forward; // S
    if (direction & 4) move -= right; // A
    if (direction & 8) move += right; // D

    float speed = sprint ? FLY_SPEED * SPRINT_MULTIPLIER : FLY_SPEED;
    moveTarget(move, speed, deltaTime);
}

void EditorOrbitCamera::applyTransform() {
    transform_.position = computePosition();
    glm::vec3 forward = glm::normalize(orbitTarget_ - transform_.position);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::cross(right, forward);
    transform_.rotation = glm::normalize(glm::quat_cast(glm::mat3(right, up, -forward)));
}
