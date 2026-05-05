#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../../../../engine/data/components/Camera.hpp"
#include "../../../../engine/data/components/Transform.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../input/PickingSystem.hpp"

class AssetManager;
class Scene;
struct AABB;
struct BoundingSphere;

using GizmoObject = std::vector<VulkanGizmoPass::GizmoVertex>;

struct GizmoTransformHandle {
    GizmoObject visualization;
    std::vector<GizmoHandle> pickingHandles;
};

class GizmosRenderer {
public:
    GizmosRenderer(AssetManager& assetManager, Scene& scene);

    GizmoObject buildGizmoLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    GizmoObject buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color);
    GizmoObject buildGizmoAABB(const AABB& aabb, const glm::vec3& color);
    GizmoObject buildGizmoObjectAABB(const std::string& objectId, const glm::vec3& color);
    GizmoObject buildGizmoBoundingSphere(const BoundingSphere& sphere, const glm::vec3& color);
    GizmoObject buildGizmoObjectBoundingSphere(const std::string& objectId, const glm::vec3& color);
    GizmoObject buildGizmoBVH(const glm::vec3& color);

    GizmoTransformHandle
    buildTranslationGizmo(const std::string& objectId, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle
    buildRotationGizmo(const std::string& objectId, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle
    buildScaleGizmo(const std::string& objectId, const Camera& camera, const Transform& cameraTransform);

private:
    void addGizmoLine(GizmoObject& gizmoObject, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

    AssetManager& assetManager_;
    Scene& scene_;
};
