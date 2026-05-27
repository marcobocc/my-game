#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "modules/scene/EntityStore.hpp"

class AssetLoader;
class EditorSettings;
struct AABB;
struct BoundingSphere;

using GizmoObject = std::vector<VulkanGizmoPass::GizmoVertex>;

class GizmoBuilder {
public:
    GizmoBuilder(AssetLoader& assetLoader, EntityManager& entityManager, EditorSettings& editorSettings);

    GizmoObject buildGizmoLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    GizmoObject buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color);
    GizmoObject buildGizmoAABB(const AABB& aabb, const glm::vec3& color);
    GizmoObject buildGizmoObjectAABB(EntityHandle objectId, const glm::vec3& color);
    GizmoObject buildGizmoBoundingSphere(const BoundingSphere& sphere, const glm::vec3& color);
    GizmoObject buildGizmoObjectBoundingSphere(EntityHandle objectId, const glm::vec3& color);
    GizmoObject buildGizmoBVH(const glm::vec3& color);

private:
    void addGizmoLine(GizmoObject& gizmoObject, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

    AssetLoader& assetLoader_;
    EntityManager& entityManager_;
    EditorSettings& editorSettings_;
};
