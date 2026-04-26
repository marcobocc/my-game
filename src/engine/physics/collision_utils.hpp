#pragma once
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include "core/objects/components/BoxCollider.hpp"
#include "core/objects/components/Transform.hpp"

namespace Physics {
    struct RaycastHit {
        std::string objectId;
        glm::vec3 point;
        float distance = 0.0f;
    };

    // Returns the world-space AABB min/max for a BoxCollider given its transform.
    // Computes the 8 corners of the oriented box and takes the per-axis extremes,
    // so it works correctly with non-uniform scale and rotation.
    inline std::pair<glm::vec3, glm::vec3> worldAABB(const BoxCollider& collider, const Transform& transform) {
        glm::mat4 model = transform.getModelMatrix();
        glm::vec3 localCenter = collider.center;
        glm::vec3 he = collider.halfExtents;
        glm::vec3 worldMin(std::numeric_limits<float>::max());
        glm::vec3 worldMax(std::numeric_limits<float>::lowest());
        for (int x: {-1, 1}) {
            for (int y: {-1, 1}) {
                for (int z: {-1, 1}) {
                    glm::vec4 corner = model * glm::vec4(localCenter + glm::vec3(x, y, z) * he, 1.0f);
                    worldMin = glm::min(worldMin, glm::vec3(corner));
                    worldMax = glm::max(worldMax, glm::vec3(corner));
                }
            }
        }
        return {worldMin, worldMax};
    }

    // Returns true if the two oriented box colliders overlap in world space.
    inline bool checkCollision(const BoxCollider& a, const Transform& ta, const BoxCollider& b, const Transform& tb) {
        auto [aMin, aMax] = worldAABB(a, ta);
        auto [bMin, bMax] = worldAABB(b, tb);
        return aMin.x <= bMax.x && aMax.x >= bMin.x && aMin.y <= bMax.y && aMax.y >= bMin.y && aMin.z <= bMax.z &&
               aMax.z >= bMin.z;
    }

    // Ray-AABB slab test. Returns the hit distance along the ray, or nullopt if no hit.
    inline std::optional<float>
    raycastBox(const glm::vec3& origin, const glm::vec3& dir, const BoxCollider& collider, const Transform& transform) {
        auto [boxMin, boxMax] = worldAABB(collider, transform);
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();
        for (int i = 0; i < 3; ++i) {
            float invD = 1.0f / dir[i];
            float t0 = (boxMin[i] - origin[i]) * invD;
            float t1 = (boxMax[i] - origin[i]) * invD;
            if (invD < 0.0f) std::swap(t0, t1);
            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMax < tMin) return std::nullopt;
        }
        return tMin;
    }

    // Casts a ray against a list of (objectId, collider, transform) tuples.
    // Returns the closest hit, or nullopt if nothing was hit.
    template<typename Range>
    std::optional<RaycastHit> raycast(const glm::vec3& origin, const glm::vec3& dir, Range&& objects) {
        std::optional<RaycastHit> closest;
        for (auto& [id, collider, transform]: objects) {
            auto dist = raycastBox(origin, dir, collider, transform);
            if (dist && (!closest || *dist < closest->distance)) {
                closest = RaycastHit{id, origin + dir * (*dist), *dist};
            }
        }
        return closest;
    }

} // namespace Physics
