#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include "AABB.hpp"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction; // must be normalized

    // Returns true and sets tHit if ray intersects aabb (tHit >= 0 means in front of camera).
    bool intersectsAABB(const AABB& aabb, float& tHit) const {
        glm::vec3 invDir = 1.0f / direction;

        float tMin = (aabb.min.x - origin.x) * invDir.x;
        float tMax = (aabb.max.x - origin.x) * invDir.x;
        if (tMin > tMax) std::swap(tMin, tMax);

        float tyMin = (aabb.min.y - origin.y) * invDir.y;
        float tyMax = (aabb.max.y - origin.y) * invDir.y;
        if (tyMin > tyMax) std::swap(tyMin, tyMax);

        if (tMin > tyMax || tyMin > tMax) return false;
        tMin = std::max(tMin, tyMin);
        tMax = std::min(tMax, tyMax);

        float tzMin = (aabb.min.z - origin.z) * invDir.z;
        float tzMax = (aabb.max.z - origin.z) * invDir.z;
        if (tzMin > tzMax) std::swap(tzMin, tzMax);

        if (tMin > tzMax || tzMin > tMax) return false;
        tMin = std::max(tMin, tzMin);
        tMax = std::min(tMax, tzMax);

        if (tMax < 0.0f) return false;
        tHit = tMin >= 0.0f ? tMin : tMax;
        return true;
    }
};
