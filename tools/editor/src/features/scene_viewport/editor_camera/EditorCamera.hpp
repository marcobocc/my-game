#pragma once
#include <glm/glm.hpp>
#include "../../../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../../../runtime/src/graphics/components/Camera.hpp"

class EditorCamera {
public:
    static constexpr float ORBIT_SENSITIVITY = 0.005f;
    static constexpr float PAN_SENSITIVITY = 0.01f;
    static constexpr float ZOOM_SENSITIVITY = 0.5f;
    static constexpr float FLY_SPEED = 5.0f;
    static constexpr float SPRINT_MULTIPLIER = 4.0f;
    static constexpr float INITIAL_DISTANCE = 6.0f;
    static constexpr float PITCH_CLAMP_MARGIN = 0.05f;

    EditorCamera();

    const Camera& getCamera() const { return camera_; }
    const Transform& getCameraTransform() const { return transform_; }
    void setAspectRatio(float aspect) { camera_.aspect = aspect; }
    void resetToDefault();
    void resetFromTransform(const Transform& t);

    void orbit(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);
    void zoom(double scrollDelta);

    void moveTarget(const glm::vec3& direction, float speed, double deltaTime);
    void moveFromKeyboard(uint8_t direction, bool sprint, double deltaTime);

private:
    Camera camera_;
    Transform transform_;

    float yaw_ = 0.0f;
    float pitch_ = 0.0f;

    void applyRotation();
};
