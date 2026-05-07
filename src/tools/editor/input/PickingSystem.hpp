#pragma once
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include <variant>
#include <vector>
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/modules/assets/AssetManager.hpp"
#include "../../../engine/modules/input/RaycastPickingSystem.hpp"
#include "../../../engine/utils/math/Ray.hpp"
#include "../gizmos.hpp"

/*
    PickingSystem

    Two-layer picking for the editor viewport:
      1. Gizmo handles (tested first — takes priority over scene objects)
      2. Scene objects via AABB raycast (falls through if no handle hit)

    Gizmo handles are registered each frame via registerHandle() before pick() is called,
    and cleared via clearHandles() at the start of each frame.
*/

using EditorPickResult = std::variant<GizmoHit, SceneObjectHit>;

class PickingSystem {
public:
    explicit PickingSystem(AssetManager& assetManager) : assetManager_(assetManager) {}

    void registerHandle(const GizmoHandle& handle) { handles_.push_back(handle); }
    void clearHandles() { handles_.clear(); }

    // Perform a two-layer pick from a viewport pixel coordinate.
    std::optional<EditorPickResult> pick(uint32_t pixelX,
                                         uint32_t pixelY,
                                         uint32_t viewportX,
                                         uint32_t viewportY,
                                         uint32_t viewportWidth,
                                         uint32_t viewportHeight,
                                         const Camera& camera,
                                         const Transform& cameraTransform,
                                         EntityManager& entityManager) const {
        if (viewportWidth == 0 || viewportHeight == 0) return std::nullopt;

        float ndcX = (static_cast<float>(pixelX) - static_cast<float>(viewportX)) / static_cast<float>(viewportWidth);
        float ndcY = (static_cast<float>(pixelY) - static_cast<float>(viewportY)) / static_cast<float>(viewportHeight);
        glm::vec2 ndc{ndcX, ndcY};

        Ray ray = RaycastPickingSystem::buildRay(ndc, camera, cameraTransform);

        // Layer 1: gizmo handles — pick the one with the smallest perpendicular miss distance
        std::optional<GizmoHit> bestHandle;
        float bestDist2 = std::numeric_limits<float>::max();
        for (const auto& handle: handles_) {
            float t = 0.0f;
            float dist2 = 0.0f;
            if (intersectsCapsule(ray, handle.base, handle.tip, handle.radius, t, dist2) && dist2 < bestDist2) {
                bestDist2 = dist2;
                bestHandle.emplace(handle.objectId, handle.type, handle.axis);
            }
        }
        if (bestHandle) return EditorPickResult{*bestHandle};

        // Layer 2: scene objects via AABB raycast
        auto hit = RaycastPickingSystem::pick(ndc, camera, cameraTransform, entityManager, assetManager_);
        if (hit) return EditorPickResult{SceneObjectHit{hit.value()}};

        return std::nullopt;
    }

private:
    AssetManager& assetManager_;
    std::vector<GizmoHandle> handles_;

    // outDist2 is the squared perpendicular distance (used for tiebreaking between overlapping handles)
    static bool intersectsCapsule(
            const Ray& ray, const glm::vec3& a, const glm::vec3& b, float radius, float& tHit, float& outDist2) {
        glm::vec3 ab = b - a;
        glm::vec3 ao = ray.origin - a;

        float abab = glm::dot(ab, ab);
        float abrd = glm::dot(ab, ray.direction);
        float abao = glm::dot(ab, ao);
        float rdao = glm::dot(ray.direction, ao);

        float denom = abab - abrd * abrd;

        float t = 0.0f;
        float s = 0.0f;

        if (std::abs(denom) < 1e-6f) {
            s = abab > 0.0f ? abao / abab : 0.0f;
        } else {
            t = (abrd * abao - abab * rdao) / denom;
            s = (abrd * t + abao) / abab;
        }

        s = glm::clamp(s, 0.0f, 1.0f);

        glm::vec3 closestOnSeg = a + s * ab;
        t = glm::max(0.0f, glm::dot(closestOnSeg - ray.origin, ray.direction));
        glm::vec3 closestOnRay = ray.origin + t * ray.direction;

        float dist2 = glm::dot(closestOnSeg - closestOnRay, closestOnSeg - closestOnRay);
        if (dist2 > radius * radius) return false;

        tHit = t;
        outDist2 = dist2;
        return true;
    }
};
