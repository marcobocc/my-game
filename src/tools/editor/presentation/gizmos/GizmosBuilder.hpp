#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../../../../engine/data/components/Camera.hpp"
#include "../../../../engine/data/components/Transform.hpp"
#include "../../../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../../engine/modules/scene/EntityMetadata.hpp"
#include "../../input/PickingSystem.hpp"

class AssetManager;
class Scene;
class EditorSettings;
struct AABB;
struct BoundingSphere;

using GizmoObject = std::vector<VulkanGizmoPass::GizmoVertex>;

struct GizmoTransformHandle {
    GizmoObject visualization;
    std::vector<GizmoHandle> pickingHandles;
};

class GizmosRenderer {
public:
    GizmosRenderer(AssetManager& assetManager, EntityManager& entityManager, EditorSettings& editorSettings);

    GizmoObject buildGizmoLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    GizmoObject buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color);
    GizmoObject buildGizmoAABB(const AABB& aabb, const glm::vec3& color);
    GizmoObject buildGizmoObjectAABB(EntityHandle objectId, const glm::vec3& color);
    GizmoObject buildGizmoBoundingSphere(const BoundingSphere& sphere, const glm::vec3& color);
    GizmoObject buildGizmoObjectBoundingSphere(EntityHandle objectId, const glm::vec3& color);
    GizmoObject buildGizmoBVH(const glm::vec3& color);

    GizmoTransformHandle
    buildTranslationGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle
    buildRotationGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle buildScaleGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform);

private:
    void addGizmoLine(GizmoObject& gizmoObject, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

    AssetManager& assetManager_;
    EntityManager& entityManager_;
    EditorSettings& editorSettings_;
};
