#pragma once
#include "BehaviourComponent.hpp"

class PlayerController final : public BehaviourComponent {
public:
    std::string typeName() const override { return "PlayerController"; }
    nlohmann::json serialize() const override { return {{"todo_", "todo_"}}; } // TODO
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<PlayerController>(*this); }

    void onStart() override {
        if (auto* t = getComponent<Transform>()) {
            glm::vec3 forward = t->getForward();
            yaw_ = glm::degrees(std::atan2(forward.x, forward.z));
            pitch_ = glm::degrees(std::asin(glm::clamp(forward.y, -1.0f, 1.0f)));
        }
        lockMouse();
        logInfo("started — F: damage, H: heal");
    }

    void onUpdate(float dt) override {
        auto* t = getComponent<Transform>();
        if (!t) return;

        auto [dx, dy] = getMouseDelta();
        yaw_ -= static_cast<float>(dx) * mouseSensitivity_;
        pitch_ -= static_cast<float>(dy) * mouseSensitivity_;
        pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);

        t->rotation = glm::angleAxis(glm::radians(yaw_), glm::vec3(0, 1, 0)) *
                      glm::angleAxis(glm::radians(pitch_), glm::vec3(1, 0, 0));

        glm::vec3 forward = t->getForward();
        forward.y = 0.0f;
        forward = glm::normalize(forward);
        glm::vec3 right = t->getRight();
        right.y = 0.0f;
        right = glm::normalize(right);

        if (isKeyDown(87)) t->position += forward * moveSpeed_ * dt; // W
        if (isKeyDown(83)) t->position -= forward * moveSpeed_ * dt; // S
        if (isKeyDown(65)) t->position += right * moveSpeed_ * dt; // A
        if (isKeyDown(68)) t->position -= right * moveSpeed_ * dt; // D

        if (grounded_ && isKeyPressed(32)) {
            verticalVelocity_ = jumpForce_ / mass_;
            grounded_ = false;
        }

        verticalVelocity_ -= gravity_ * dt;
        t->position.y += verticalVelocity_ * dt;
        if (t->position.y <= height_) {
            t->position.y = height_;
            verticalVelocity_ = 0.0f;
            grounded_ = true;
        }

        if (auto* health = getComponent<HealthComponent>()) {
            if (isKeyPressed(70)) health->takeDamage(10.0f); // F
            if (isKeyPressed(72)) health->heal(10.0f); // H
        }
    }

public:
    float moveSpeed_ = 7.0f;
    float mouseSensitivity_ = 0.1f;
    float jumpForce_ = 800.0f;
    float gravity_ = 9.81f;
    float mass_ = 70.0f;
    float height_ = 1.7f;

private:
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float verticalVelocity_ = 0.0f;
    bool grounded_ = true;
};
