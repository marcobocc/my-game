#include "GizmoBuilder.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../../../services/EditorSettings.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/asset_types/Mesh.hpp"
#include "modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "modules/scene/EntityStore.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "utils/math/AABB.hpp"
#include "utils/math/BVH.hpp"
#include "utils/math/BoundingSphere.hpp"

GizmoBuilder::GizmoBuilder(AssetLoader& assetLoader, EntityManager& entityManager, EditorSettings& editorSettings) :
    assetLoader_(assetLoader),
    entityManager_(entityManager),
    editorSettings_(editorSettings) {}

void GizmoBuilder::addGizmoLine(GizmoObject& gizmoObject,
                                const glm::vec3& from,
                                const glm::vec3& to,
                                const glm::vec3& color) {
    if (gizmoObject.size() >= VulkanGizmoPass::MAX_LINES * 2) return;
    gizmoObject.push_back({from, color});
    gizmoObject.push_back({to, color});
}

static glm::vec3 rotateVector(const glm::vec3& vector, const glm::quat& rotation) {
    glm::mat3 rotationMatrix = glm::mat3_cast(rotation);
    return rotationMatrix * vector;
}

GizmoObject GizmoBuilder::buildGizmoLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    GizmoObject gizmoObject;
    addGizmoLine(gizmoObject, from, to, color);
    return gizmoObject;
}

GizmoObject GizmoBuilder::buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color) {
    GizmoObject gizmoObject;
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
    addGizmoLine(gizmoObject, c[0], c[1], color);
    addGizmoLine(gizmoObject, c[1], c[2], color);
    addGizmoLine(gizmoObject, c[2], c[3], color);
    addGizmoLine(gizmoObject, c[3], c[0], color);
    addGizmoLine(gizmoObject, c[4], c[5], color);
    addGizmoLine(gizmoObject, c[5], c[6], color);
    addGizmoLine(gizmoObject, c[6], c[7], color);
    addGizmoLine(gizmoObject, c[7], c[4], color);
    addGizmoLine(gizmoObject, c[0], c[4], color);
    addGizmoLine(gizmoObject, c[1], c[5], color);
    addGizmoLine(gizmoObject, c[2], c[6], color);
    addGizmoLine(gizmoObject, c[3], c[7], color);
    return gizmoObject;
}

GizmoObject GizmoBuilder::buildGizmoAABB(const AABB& aabb, const glm::vec3& color) {
    GizmoObject gizmoObject;
    const glm::vec3 min = aabb.min;
    const glm::vec3 max = aabb.max;

    glm::vec3 v000 = {min.x, min.y, min.z};
    glm::vec3 v001 = {min.x, min.y, max.z};
    glm::vec3 v010 = {max.x, min.y, min.z};
    glm::vec3 v011 = {max.x, min.y, max.z};

    glm::vec3 v100 = {min.x, max.y, min.z};
    glm::vec3 v101 = {min.x, max.y, max.z};
    glm::vec3 v110 = {max.x, max.y, min.z};
    glm::vec3 v111 = {max.x, max.y, max.z};

    addGizmoLine(gizmoObject, v000, v010, color);
    addGizmoLine(gizmoObject, v010, v011, color);
    addGizmoLine(gizmoObject, v011, v001, color);
    addGizmoLine(gizmoObject, v001, v000, color);

    addGizmoLine(gizmoObject, v100, v110, color);
    addGizmoLine(gizmoObject, v110, v111, color);
    addGizmoLine(gizmoObject, v111, v101, color);
    addGizmoLine(gizmoObject, v101, v100, color);

    addGizmoLine(gizmoObject, v000, v100, color);
    addGizmoLine(gizmoObject, v001, v101, color);
    addGizmoLine(gizmoObject, v010, v110, color);
    addGizmoLine(gizmoObject, v011, v111, color);
    return gizmoObject;
}

GizmoObject GizmoBuilder::buildGizmoObjectAABB(EntityHandle objectId, const glm::vec3& color) {
    auto* transform = entityManager_.getComponent<Transform>(objectId);
    auto* renderer = entityManager_.getComponent<Renderer>(objectId);
    if (transform && renderer) {
        auto mesh = assetLoader_.get<Mesh>(renderer->meshName);
        auto aabb = mesh->getAABB().applyTransform(transform->getModelMatrix());
        return buildGizmoAABB(aabb, color);
    }
    return {};
}

GizmoObject GizmoBuilder::buildGizmoBoundingSphere(const BoundingSphere& sphere, const glm::vec3& color) {
    GizmoObject gizmoObject;
    constexpr int segments = 48;
    const glm::vec3 perp1s[3] = {{0, 1, 0}, {1, 0, 0}, {1, 0, 0}};
    const glm::vec3 perp2s[3] = {{0, 0, 1}, {0, 0, 1}, {0, 1, 0}};

    for (int i = 0; i < 3; ++i) {
        glm::vec3 p1 = perp1s[i];
        glm::vec3 p2 = perp2s[i];

        glm::vec3 prevPt{};
        for (int s = 0; s <= segments; ++s) {
            float angle = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
            glm::vec3 pt = sphere.center + (p1 * glm::cos(angle) + p2 * glm::sin(angle)) * sphere.radius;
            if (s > 0) {
                addGizmoLine(gizmoObject, prevPt, pt, color);
            }
            prevPt = pt;
        }
    }
    return gizmoObject;
}

GizmoObject GizmoBuilder::buildGizmoObjectBoundingSphere(EntityHandle objectId, const glm::vec3& color) {
    auto* transform = entityManager_.getComponent<Transform>(objectId);
    auto* renderer = entityManager_.getComponent<Renderer>(objectId);
    if (transform && renderer) {
        auto mesh = assetLoader_.get<Mesh>(renderer->meshName);
        auto sphere = mesh->getBoundingSphere().applyTransform(transform->getModelMatrix());
        return buildGizmoBoundingSphere(sphere, color);
    }
    return {};
}

GizmoObject GizmoBuilder::buildGizmoBVH(const glm::vec3& color) {
    GizmoObject gizmoObject;
    std::vector<Item> items;
    auto renderables = entityManager_.query<Transform, Renderer>();
    for (const auto& [entity, transform, renderer]: renderables) {
        auto* mesh = assetLoader_.get<Mesh>(renderer->meshName);
        if (!mesh) continue;
        auto aabb = mesh->getAABB().applyTransform(transform->getModelMatrix());
        items.emplace_back(aabb, std::to_string(entity));
    }
    if (items.empty()) return gizmoObject;
    BVH bvh;
    bvh.build(std::move(items));
    for (const auto& node: bvh.nodes) {
        auto nodeGizmo = buildGizmoAABB(node.bounds, color);
        gizmoObject.insert(gizmoObject.end(), nodeGizmo.begin(), nodeGizmo.end());
        if (node.isLeaf()) {
            for (uint32_t i = node.begin; i < node.begin + node.count; i++) {
                auto itemGizmo = buildGizmoAABB(bvh.items[i].aabb, color);
                gizmoObject.insert(gizmoObject.end(), itemGizmo.begin(), itemGizmo.end());
            }
        }
    }
    return gizmoObject;
}
