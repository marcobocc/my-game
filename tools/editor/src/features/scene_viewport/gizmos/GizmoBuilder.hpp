#pragma once
#include <glm/glm.hpp>
#include "modules/scene/World.hpp"

class AssetLoader;
class EditorSettings;
class DebugDraw;
struct AABB;
struct BoundingSphere;

class GizmoBuilder {
public:
    GizmoBuilder(AssetLoader& assetLoader, World& entityManager, EditorSettings& editorSettings);

    void buildGizmoLine(DebugDraw& out, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    void buildGizmoCube(DebugDraw& out, const glm::vec3& center, float halfSize, const glm::vec3& color);
    void buildGizmoAABB(DebugDraw& out, const AABB& aabb, const glm::vec3& color);
    void buildGizmoObjectAABB(DebugDraw& out, EntityHandle objectId, const World& world, const glm::vec3& color);
    void buildGizmoObjectBoxCollider(DebugDraw& out, EntityHandle objectId, const World& world, const glm::vec3& color);
    void
    buildGizmoObjectCapsuleCollider(DebugDraw& out, EntityHandle objectId, const World& world, const glm::vec3& color);
    void buildGizmoBoundingSphere(DebugDraw& out, const BoundingSphere& sphere, const glm::vec3& color);
    void
    buildGizmoObjectBoundingSphere(DebugDraw& out, EntityHandle objectId, const World& world, const glm::vec3& color);
    void buildGizmoBVH(DebugDraw& out, const World& world, const glm::vec3& color);

private:
    AssetLoader& assetLoader_;
    World& entityManager_;
    EditorSettings& editorSettings_;
};
