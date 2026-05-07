#pragma once
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include "../../data/assets/Mesh.hpp"
#include "../../data/components/Camera.hpp"
#include "../../data/components/Renderer.hpp"
#include "../../data/components/Transform.hpp"
#include "../../modules/assets/AssetManager.hpp"
#include "../../modules/scene/EntityManager.hpp"
#include "../../utils/math/Ray.hpp"

/*
    RaycastPickingSystem

    Generic engine utility that casts a ray from a camera through a screen-space
    pixel and returns the name of the closest scene object whose AABB is hit.

    Suitable for use by both the editor and in-game code.
*/
class RaycastPickingSystem {
public:
    // ndcPos: [0,1] screen-space coord (x right, y down from top-left of the viewport)
    static std::optional<EntityHandle> pick(const glm::vec2& ndcPos,
                                            const Camera& camera,
                                            const Transform& cameraTransform,
                                            EntityManager& entityManager,
                                            AssetManager& assetManager) {
        Ray ray = buildRay(ndcPos, camera, cameraTransform);

        std::optional<EntityHandle> best;
        float bestT = std::numeric_limits<float>::max();

        auto objects = entityManager.query<Transform, Renderer>();
        for (auto& [entity, transformPtr, rendererPtr]: objects) {
            if (!transformPtr || !rendererPtr) continue;
            if (!rendererPtr->enabled) continue;
            const Mesh* mesh = assetManager.get<Mesh>(rendererPtr->meshName);
            if (!mesh) continue;

            AABB worldAABB = mesh->getAABB().applyTransform(transformPtr->getModelMatrix());

            float t = 0.0f;
            if (ray.intersectsAABB(worldAABB, t) && t < bestT) {
                bestT = t;
                best.emplace(entity);
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
                                                     EntityManager& entityManager,
                                                     AssetManager& assetManager) {
        if (viewportWidth == 0 || viewportHeight == 0) return std::nullopt;
        float ndcX = (static_cast<float>(pixelX) - static_cast<float>(viewportX)) / static_cast<float>(viewportWidth);
        float ndcY = (static_cast<float>(pixelY) - static_cast<float>(viewportY)) / static_cast<float>(viewportHeight);
        return pick({ndcX, ndcY}, camera, cameraTransform, entityManager, assetManager);
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
