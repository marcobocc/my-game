#pragma once
#include "core/scene/Scene.hpp"
#include "physics/collision_utils.hpp"

class PhysicsSystem {
public:
    explicit PhysicsSystem(Scene& scene) : scene_(scene) {}

    bool checkCollision(const BoxCollider& a, const Transform& ta, const BoxCollider& b, const Transform& tb) const {
        return Physics::checkCollision(a, ta, b, tb);
    }

    std::optional<Physics::RaycastHit> raycast(const glm::vec3& origin, const glm::vec3& dir) const {
        auto objects = scene_.getObjectsWith<BoxCollider, Transform>();
        return Physics::raycast(origin, dir, objects);
    }

private:
    Scene& scene_;
};
