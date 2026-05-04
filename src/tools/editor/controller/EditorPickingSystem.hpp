#pragma once
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/systems/assets/AssetManager.hpp"
#include "../../../engine/systems/input/RaycastPickingSystem.hpp"
#include "../../../engine/systems/scene/Scene.hpp"
#include "../../../engine/utils/math/Ray.hpp"

/*
    EditorPickingSystem

    Two-layer picking for the editor viewport:
      1. Gizmo handles (tested first — takes priority over scene objects)
      2. Scene objects via AABB raycast (falls through if no handle hit)

    Gizmo handles are registered each frame via registerHandle() before pick() is called,
    and cleared via clearHandles() at the start of each frame.
*/

enum class GizmoAxis { X, Y, Z };
enum class GizmoType { Translation, Rotation, Scale };

struct GizmoHit {
    GizmoType type;
    GizmoAxis axis;
};

struct SceneObjectHit {
    std::string objectId;
};

using EditorPickResult = std::variant<GizmoHit, SceneObjectHit>;

// TODO: placeholder — will hold pickable capsule geometry for each gizmo handle (translation/rotation/scale)
struct GizmoHandle {
    GizmoType type;
    GizmoAxis axis;
    glm::vec3 base;
    glm::vec3 tip;
    float radius;
};

class EditorPickingSystem {
public:
    explicit EditorPickingSystem(AssetManager& assetManager) : assetManager_(assetManager) {}

    // TODO: placeholder — called by gizmo drawing code each frame to register pickable handles
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
                                         Scene& scene) const {
        if (viewportWidth == 0 || viewportHeight == 0) return std::nullopt;

        float ndcX = (static_cast<float>(pixelX) - static_cast<float>(viewportX)) / static_cast<float>(viewportWidth);
        float ndcY = (static_cast<float>(pixelY) - static_cast<float>(viewportY)) / static_cast<float>(viewportHeight);
        glm::vec2 ndc{ndcX, ndcY};

        Ray ray = RaycastPickingSystem::buildRay(ndc, camera, cameraTransform);

        // Layer 1: gizmo handles (closest hit wins)
        std::optional<GizmoHit> bestHandle;
        float bestHandleT = std::numeric_limits<float>::max();
        for (const auto& handle: handles_) {
            float t = 0.0f;
            if (intersectsCapsule(ray, handle.base, handle.tip, handle.radius, t) && t < bestHandleT) {
                bestHandleT = t;
                bestHandle = GizmoHit{handle.type, handle.axis};
            }
        }
        if (bestHandle) return EditorPickResult{*bestHandle};

        // Layer 2: scene objects via AABB raycast
        auto hit = RaycastPickingSystem::pick(ndc, camera, cameraTransform, scene, assetManager_);
        if (hit) return EditorPickResult{SceneObjectHit{*hit}};

        return std::nullopt;
    }

private:
    AssetManager& assetManager_;
    std::vector<GizmoHandle> handles_; // TODO: placeholder — populated each frame by gizmo drawing code

    // TODO: placeholder — ray-capsule test used for gizmo handle picking
    static bool intersectsCapsule(const Ray& ray, const glm::vec3& a, const glm::vec3& b, float radius, float& tHit) {
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

        // Closest point on segment to the ray
        glm::vec3 closestOnSeg = a + s * ab;

        // Closest point on ray to that segment point
        t = glm::max(0.0f, glm::dot(closestOnSeg - ray.origin, ray.direction));
        glm::vec3 closestOnRay = ray.origin + t * ray.direction;

        float dist2 = glm::dot(closestOnSeg - closestOnRay, closestOnSeg - closestOnRay);
        if (dist2 > radius * radius) return false;

        tHit = t;
        return true;
    }
};
