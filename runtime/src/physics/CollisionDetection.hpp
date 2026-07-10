#pragma once
#include <array>
#include <glm/glm.hpp>
#include <optional>
#include "../core/components/Metadata.hpp"
#include "../core/components/Transform.hpp"
#include "../core/scene/EntityHandle.hpp"
#include "../core/scene/TransformUtils.hpp"
#include "components/BoxCollider.hpp"
#include "components/CapsuleCollider.hpp"

namespace Physics {
    struct RaycastHit {
        EntityHandle objectId;
        glm::vec3 point;
        float distance = 0.0f;
    };

    // Returns the world-space AABB min/max for a BoxCollider given its world matrix.
    // Computes the 8 corners of the oriented box and takes the per-axis extremes,
    // so it works correctly with non-uniform scale and rotation.
    inline std::pair<glm::vec3, glm::vec3> worldAABB(const BoxCollider& collider, const glm::mat4& model) {
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

    // SAT OBB vs OBB MTV. Tests all 15 separating axes (3+3 face normals + 9 edge crosses).
    // Returns the minimum translation vector to push A out of B, or nullopt if no overlap.
    inline std::optional<glm::vec3>
    computeMTV(const BoxCollider& a, const glm::mat4& matA, const BoxCollider& b, const glm::mat4& matB) {
        // Extract world-space center and axes (columns of rotation, scaled by half-extents).
        constexpr float kEps = 1e-6f;

        struct OBB {
            glm::vec3 center{};
            std::array<glm::vec3, 3> ax{};
            std::array<glm::vec3, 3> he{};
        };
        auto extractOBB = [](const BoxCollider& c, const glm::mat4& m) {
            OBB obb;
            obb.center = glm::vec3(m * glm::vec4(c.center, 1.0f));
            for (int i = 0; i < 3; ++i) {
                glm::vec3 col = glm::vec3(m[i]);
                float len = glm::length(col);
                obb.ax.at(i) = (len > 0.0f) ? col / len : glm::vec3(0.0f);
                obb.he.at(i) = obb.ax.at(i) * (c.halfExtents[i] * len);
            }
            return obb;
        };

        OBB obbA = extractOBB(a, matA);
        OBB obbB = extractOBB(b, matB);
        const auto& axA = obbA.ax;
        const auto& heA = obbA.he;
        const auto& axB = obbB.ax;
        const auto& heB = obbB.he;

        glm::vec3 d = obbB.center - obbA.center;

        auto project = [&](const glm::vec3& axis) -> float {
            float pA = 0.0f, pB = 0.0f;
            for (const auto& h: heA)
                pA += std::abs(glm::dot(h, axis));
            for (const auto& h: heB)
                pB += std::abs(glm::dot(h, axis));
            return pA + pB;
        };

        float bestOverlap = std::numeric_limits<float>::max();
        glm::vec3 bestAxis{0.0f};

        auto test = [&](glm::vec3 axis) -> bool {
            float len = glm::length(axis);
            if (len < kEps) return true;
            axis /= len;
            float dist = std::abs(glm::dot(d, axis));
            float overlap = project(axis) - dist;
            if (overlap <= 0.0f) return false;
            if (overlap < bestOverlap) {
                bestOverlap = overlap;
                bestAxis = (glm::dot(d, axis) >= 0.0f) ? -axis : axis;
            }
            return true;
        };

        for (const auto& ax: axA)
            if (!test(ax)) return std::nullopt;
        for (const auto& ax: axB)
            if (!test(ax)) return std::nullopt;
        for (const auto& ai: axA)
            for (const auto& bi: axB)
                if (!test(glm::cross(ai, bi))) return std::nullopt;

        return bestAxis * bestOverlap;
    }

    // Capsule vs OBB MTV. Finds the closest point on the capsule's segment to the OBB,
    // then checks if a sphere of 'radius' at that point penetrates the OBB.
    // Returns the MTV to push the capsule out, or nullopt if no overlap.
    inline std::optional<glm::vec3> computeCapsuleBoxMTV(const CapsuleCollider& cap,
                                                         const glm::mat4& matCap,
                                                         const BoxCollider& box,
                                                         const glm::mat4& matBox) {
        // World-space capsule segment
        glm::vec3 capWorldCenter = glm::vec3(matCap * glm::vec4(cap.center, 1.0f));
        glm::vec3 up = glm::normalize(glm::vec3(matCap * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
        float scaleY = glm::length(glm::vec3(matCap[1]));
        float scaleXZ = glm::length(glm::vec3(matCap[0]));
        float halfH = cap.height * 0.5f * scaleY;
        float r = cap.radius * scaleXZ;
        glm::vec3 segA = capWorldCenter + up * halfH; // top hemisphere center
        glm::vec3 segB = capWorldCenter - up * halfH; // bottom hemisphere center

        // OBB in world space
        glm::vec3 boxCenter = glm::vec3(matBox * glm::vec4(box.center, 1.0f));
        std::array<glm::vec3, 3> boxAx{};
        std::array<glm::vec3, 3> boxHe{};
        for (int i = 0; i < 3; ++i) {
            glm::vec3 col = glm::vec3(matBox[i]);
            float len = glm::length(col);
            boxAx.at(i) = (len > 1e-6f) ? col / len : glm::vec3(0.0f);
            boxHe.at(i) = boxAx.at(i) * (box.halfExtents[i] * len);
        }

        // Find closest point on segment [segA, segB] to OBB
        // Project segment into OBB local space, clamp, project back
        glm::vec3 d = segB - segA;
        // Closest point on segment to box center: parameterize t in [0,1]
        // Use iterative: find closest point on segment to OBB by finding closest OBB point to segment
        // Simple approach: closest point on segment to box center, then clamp to OBB
        float segLen = glm::length(d);
        glm::vec3 segDir = (segLen > 1e-6f) ? d / segLen : glm::vec3(0.0f, 1.0f, 0.0f);

        // Project box center onto segment
        float t = glm::dot(boxCenter - segA, segDir);
        t = glm::clamp(t, 0.0f, segLen);
        glm::vec3 closestOnSeg = segA + segDir * t;

        // Find closest point on OBB to closestOnSeg
        glm::vec3 diff = closestOnSeg - boxCenter;
        glm::vec3 closestOnBox = boxCenter;
        for (int i = 0; i < 3; ++i) {
            float he = glm::length(boxHe[i]);
            float proj = glm::clamp(glm::dot(diff, boxAx[i]), -he, he);
            closestOnBox += boxAx[i] * proj;
        }

        // Re-project segment to get truly closest point (one iteration is usually sufficient)
        t = glm::dot(closestOnBox - segA, segDir);
        t = glm::clamp(t, 0.0f, segLen);
        closestOnSeg = segA + segDir * t;

        diff = closestOnSeg - boxCenter;
        closestOnBox = boxCenter;
        for (int i = 0; i < 3; ++i) {
            float he = glm::length(boxHe[i]);
            float proj = glm::clamp(glm::dot(diff, boxAx[i]), -he, he);
            closestOnBox += boxAx[i] * proj;
        }

        glm::vec3 sep = closestOnSeg - closestOnBox;
        float dist = glm::length(sep);
        if (dist >= r) return std::nullopt;

        // Check if capsule center is inside OBB (dist == 0)
        if (dist < 1e-6f) {
            // Find least-penetrating face axis to push out
            float bestPen = std::numeric_limits<float>::max();
            glm::vec3 bestAxis{0.0f, 1.0f, 0.0f};
            glm::vec3 toCenter = closestOnSeg - boxCenter;
            for (int i = 0; i < 3; ++i) {
                float he = glm::length(boxHe[i]);
                float pen = he - std::abs(glm::dot(toCenter, boxAx[i]));
                if (pen < bestPen) {
                    bestPen = pen;
                    float sign = (glm::dot(toCenter, boxAx[i]) >= 0.0f) ? 1.0f : -1.0f;
                    bestAxis = boxAx[i] * sign;
                }
            }
            return bestAxis * (r + bestPen);
        }

        return glm::normalize(sep) * (r - dist);
    }

    // Returns true if the two oriented box colliders overlap in world space (SAT OBB test).
    inline bool
    checkCollision(const BoxCollider& a, const glm::mat4& matA, const BoxCollider& b, const glm::mat4& matB) {
        return computeMTV(a, matA, b, matB).has_value();
    }

    // Ray-AABB slab test. Returns the hit distance along the ray, or nullopt if no hit.
    inline std::optional<float>
    raycastBox(const glm::vec3& origin, const glm::vec3& dir, const BoxCollider& collider, const glm::mat4& model) {
        auto [boxMin, boxMax] = worldAABB(collider, model);
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

    // Casts a ray against a list of (entity, collider, transform) tuples.
    // Returns the closest hit, or nullopt if nothing was hit.
    template<typename Range>
    std::optional<RaycastHit>
    raycast(const glm::vec3& origin, const glm::vec3& dir, Range&& objects, const World& world) {
        std::optional<RaycastHit> closest;
        for (auto& [entity, collider, transform]: objects) {
            glm::mat4 mat = TransformUtils::resolveWorldMatrix(*transform, world);
            auto dist = raycastBox(origin, dir, *collider, mat);
            if (dist && (!closest || *dist < closest->distance)) {
                closest.emplace(entity, origin + dir * (*dist), *dist);
            }
        }
        return closest;
    }

} // namespace Physics
