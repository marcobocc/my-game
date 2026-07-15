#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include "../core/assets/AssetLoader.hpp"
#include "../core/components/Transform.hpp"
#include "../core/scene/TransformUtils.hpp"
#include "../core/scene/World.hpp"
#include "../graphics/assets/Mesh.hpp"
#include "../graphics/components/Camera.hpp"
#include "../graphics/components/Renderer.hpp"
#include "../math/Ray.hpp"

/*
    RaycastPickingSystem

    Generic engine utility that casts a ray from a camera through a screen-space
    pixel and returns the name of the closest scene object whose mesh is hit.

    Picking is per-triangle (not just AABB), so objects resting on top of another
    mesh (e.g. a terrain) are still picked correctly when they're the nearer surface
    at the clicked pixel — an object's AABB is only used as a broad-phase reject.

    Suitable for use by both the editor and in-game code.
*/
class RaycastPickingSystem {
public:
    // Moller-Trumbore ray-triangle intersection; v0/v1/v2 and the ray must be in the same space.
    static std::optional<float> intersectTriangle(const glm::vec3& origin,
                                                  const glm::vec3& dir,
                                                  const glm::vec3& v0,
                                                  const glm::vec3& v1,
                                                  const glm::vec3& v2) {
        constexpr float EPSILON = 1e-6f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(dir, edge2);
        float a = glm::dot(edge1, h);
        if (std::abs(a) < EPSILON) return std::nullopt;

        float f = 1.0f / a;
        glm::vec3 s = origin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return std::nullopt;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) return std::nullopt;

        float t = f * glm::dot(edge2, q);
        if (t < EPSILON) return std::nullopt;
        return t;
    }

    // ndcPos: [0,1] screen-space coord (x right, y down from top-left of the viewport)
    static std::optional<EntityHandle> pick(const glm::vec2& ndcPos,
                                            const Camera& camera,
                                            const Transform& cameraTransform,
                                            World& entityManager,
                                            AssetLoader& assetLoader) {
        Ray ray = buildRay(ndcPos, camera, cameraTransform);

        std::optional<EntityHandle> best;
        float bestT = std::numeric_limits<float>::max();

        auto objects = entityManager.query<Transform, Renderer>();
        for (auto& [entity, transformPtr, rendererPtr]: objects) {
            if (!transformPtr || !rendererPtr) continue;
            if (!rendererPtr->enabled) continue;
            const Mesh* mesh = assetLoader.get<Mesh>(rendererPtr->meshName);
            if (!mesh) continue;

            glm::mat4 model = TransformUtils::resolveWorldMatrix(*transformPtr, entityManager);
            AABB worldAABB = mesh->getAABB().applyTransform(model);

            // Broad-phase reject: skip meshes whose bounding box can't possibly beat the
            // current best hit before paying for a per-triangle test.
            float aabbT = 0.0f;
            if (!ray.intersectsAABB(worldAABB, aabbT) || aabbT >= bestT) continue;

            const auto& positions = mesh->getPositions();
            const auto& indices = mesh->getIndices();
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                glm::vec3 v0 = glm::vec3(model * glm::vec4(positions[indices[i]], 1.0f));
                glm::vec3 v1 = glm::vec3(model * glm::vec4(positions[indices[i + 1]], 1.0f));
                glm::vec3 v2 = glm::vec3(model * glm::vec4(positions[indices[i + 2]], 1.0f));
                auto t = intersectTriangle(ray.origin, ray.direction, v0, v1, v2);
                if (t && *t < bestT) {
                    bestT = *t;
                    best.emplace(entity);
                }
            }
        }

        return best;
    }

    // Converts a viewport pixel (pixelX, pixelY) to [0,1] NDC within the viewport region and
    // calls pick(). viewportX/Y are the viewport's top-left corner in window pixel space.
    static std::optional<EntityHandle> pickFromPixel(uint32_t pixelX,
                                                     uint32_t pixelY,
                                                     uint32_t viewportX,
                                                     uint32_t viewportY,
                                                     uint32_t viewportWidth,
                                                     uint32_t viewportHeight,
                                                     const Camera& camera,
                                                     const Transform& cameraTransform,
                                                     World& entityManager,
                                                     AssetLoader& assetLoader) {
        if (viewportWidth == 0 || viewportHeight == 0) return std::nullopt;
        float ndcX = (static_cast<float>(pixelX) - static_cast<float>(viewportX)) / static_cast<float>(viewportWidth);
        float ndcY = (static_cast<float>(pixelY) - static_cast<float>(viewportY)) / static_cast<float>(viewportHeight);
        return pick({ndcX, ndcY}, camera, cameraTransform, entityManager, assetLoader);
    }

    static Ray buildRay(const glm::vec2& ndcPos, const Camera& camera, const Transform& cameraTransform) {
        // Convert [0,1] viewport-space to clip-space NDC [-1,1].
        float clipX = ndcPos.x * 2.0f - 1.0f;
        float clipY = ndcPos.y * 2.0f - 1.0f;

        glm::mat4 proj = camera.getProjectionMatrix();
        glm::mat4 view = cameraTransform.getViewMatrix();
        glm::mat4 invVP = glm::inverse(proj * view);

        glm::vec4 nearPoint = invVP * glm::vec4(clipX, clipY, -1.0f, 1.0f);
        glm::vec4 farPoint = invVP * glm::vec4(clipX, clipY, 1.0f, 1.0f);

        glm::vec3 nearWorld = glm::vec3(nearPoint) / nearPoint.w;
        glm::vec3 farWorld = glm::vec3(farPoint) / farPoint.w;

        return {cameraTransform.position, glm::normalize(farWorld - nearWorld)};
    }
};
