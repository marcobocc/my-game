#include "GizmoBuilder.hpp"
#include "../../../../../../runtime/src/graphics/assets/Mesh.hpp"
#include "../../../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../../../../../../runtime/src/graphics/debug/DebugDraw.hpp"
#include "../../../../../../runtime/src/physics/MeshColliderUtils.hpp"
#include "../../../../../../runtime/src/physics/components/BoxCollider.hpp"
#include "../../../services/EditorSettings.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/scene/TransformUtils.hpp"
#include "core/scene/World.hpp"
#include "math/AABB.hpp"
#include "math/BVH.hpp"
#include "math/BoundingSphere.hpp"

GizmoBuilder::GizmoBuilder(AssetLoader& assetLoader, World& entityManager, EditorSettings& editorSettings) :
    assetLoader_(assetLoader),
    entityManager_(entityManager),
    editorSettings_(editorSettings) {}

void GizmoBuilder::buildGizmoLine(DebugDraw& out, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    out.line(from, to, color);
}

void GizmoBuilder::buildGizmoCube(DebugDraw& out, const glm::vec3& center, float halfSize, const glm::vec3& color) {
    float h = halfSize;
    glm::vec3 c[8] = {
            center + glm::vec3(-h, -h, -h),
            center + glm::vec3(h, -h, -h),
            center + glm::vec3(h, h, -h),
            center + glm::vec3(-h, h, -h),
            center + glm::vec3(-h, -h, h),
            center + glm::vec3(h, -h, h),
            center + glm::vec3(h, h, h),
            center + glm::vec3(-h, h, h),
    };
    out.line(c[0], c[1], color);
    out.line(c[1], c[2], color);
    out.line(c[2], c[3], color);
    out.line(c[3], c[0], color);
    out.line(c[4], c[5], color);
    out.line(c[5], c[6], color);
    out.line(c[6], c[7], color);
    out.line(c[7], c[4], color);
    out.line(c[0], c[4], color);
    out.line(c[1], c[5], color);
    out.line(c[2], c[6], color);
    out.line(c[3], c[7], color);
}

void GizmoBuilder::buildGizmoAABB(DebugDraw& out, const AABB& aabb, const glm::vec3& color) { out.aabb(aabb, color); }

void GizmoBuilder::buildGizmoObjectAABB(DebugDraw& out,
                                        EntityHandle objectId,
                                        const World& world,
                                        const glm::vec3& color) {
    auto* actor = world.getActor(objectId);
    if (!actor) return;
    auto* transform = actor->getComponent<Transform>();
    auto* renderer = actor->getComponent<Renderer>();
    if (!transform || !renderer) return;
    auto mesh = assetLoader_.get<Mesh>(renderer->meshName);
    auto aabb = mesh->getAABB().applyTransform(TransformUtils::resolveWorldMatrix(*transform, world));
    out.aabb(aabb, color);
}

void GizmoBuilder::buildGizmoObjectBoxCollider(DebugDraw& out,
                                               EntityHandle objectId,
                                               const World& world,
                                               const glm::vec3& color) {
    out.entityBoxCollider(objectId, world, color);
}

void GizmoBuilder::buildGizmoObjectCapsuleCollider(DebugDraw& out,
                                                   EntityHandle objectId,
                                                   const World& world,
                                                   const glm::vec3& color) {
    out.entityCapsuleCollider(objectId, world, color);
}

void GizmoBuilder::buildGizmoObjectMeshCollider(DebugDraw& out,
                                                EntityHandle objectId,
                                                const World& /*world*/,
                                                const glm::vec3& color) {
    auto* actor = entityManager_.getActor(objectId);
    if (!actor) return;
    auto* transform = actor->getComponent<Transform>();
    auto* collider = actor->getComponent<MeshCollider>();
    if (!transform || !collider) return;

    std::string meshName = Physics::resolveMeshColliderMeshName(*actor, *collider);
    const Mesh* mesh = meshName.empty() ? nullptr : assetLoader_.get<Mesh>(meshName);
    Physics::ensureMeshColliderHull(*collider, mesh);
    if (collider->hull.vertices.empty()) return;

    out.hull(TransformUtils::resolveWorldMatrix(*transform, entityManager_), collider->hull, color);
}

void GizmoBuilder::buildGizmoBoundingSphere(DebugDraw& out, const BoundingSphere& sphere, const glm::vec3& color) {
    out.sphere(sphere, color);
}

void GizmoBuilder::buildGizmoObjectBoundingSphere(DebugDraw& out,
                                                  EntityHandle objectId,
                                                  const World& world,
                                                  const glm::vec3& color) {
    auto* actor = world.getActor(objectId);
    if (!actor) return;
    auto* transform = actor->getComponent<Transform>();
    auto* renderer = actor->getComponent<Renderer>();
    if (!transform || !renderer) return;
    auto mesh = assetLoader_.get<Mesh>(renderer->meshName);
    auto sphere = mesh->getBoundingSphere().applyTransform(TransformUtils::resolveWorldMatrix(*transform, world));
    out.sphere(sphere, color);
}

void GizmoBuilder::buildGizmoBVH(DebugDraw& out, const World& world, const glm::vec3& color) {
    std::vector<Item> items;
    auto collidables = world.query<Transform, BoxCollider>();
    for (const auto& [entity, transform, boxCollider]: collidables) {
        const glm::mat4 model = TransformUtils::resolveWorldMatrix(*transform, world);
        const glm::vec3& c = boxCollider->center;
        const glm::vec3& h = boxCollider->halfExtents;
        std::vector<glm::vec3> worldCorners = {
                glm::vec3(model * glm::vec4(c.x - h.x, c.y - h.y, c.z - h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x + h.x, c.y - h.y, c.z - h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x + h.x, c.y + h.y, c.z - h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x - h.x, c.y + h.y, c.z - h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x - h.x, c.y - h.y, c.z + h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x + h.x, c.y - h.y, c.z + h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x + h.x, c.y + h.y, c.z + h.z, 1.0f)),
                glm::vec3(model * glm::vec4(c.x - h.x, c.y + h.y, c.z + h.z, 1.0f)),
        };
        items.emplace_back(AABB(worldCorners), std::to_string(entity));
    }
    if (items.empty()) return;
    BVH bvh;
    bvh.build(std::move(items));
    for (const auto& node: bvh.nodes) {
        out.aabb(node.bounds, color);
        if (node.isLeaf()) {
            for (uint32_t i = node.begin; i < node.begin + node.count; i++)
                out.aabb(bvh.items[i].aabb, color);
        }
    }
}
