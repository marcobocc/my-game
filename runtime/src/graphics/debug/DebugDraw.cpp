#include "DebugDraw.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../../core/components/Transform.hpp"
#include "../../math/BVH.hpp"
#include "../../physics/components/BoxCollider.hpp"
#include "../../physics/components/CapsuleCollider.hpp"
#include "core/scene/TransformUtils.hpp"
#include "core/scene/World.hpp"

void DebugDraw::addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    if (lines_.size() >= VulkanGizmoPass::MAX_LINES * 2) return;
    lines_.push_back({from, color});
    lines_.push_back({to, color});
}

void DebugDraw::line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) { addLine(from, to, color); }

void DebugDraw::aabb(const AABB& box, const glm::vec3& color) {
    const glm::vec3 mn = box.min;
    const glm::vec3 mx = box.max;

    glm::vec3 v000 = {mn.x, mn.y, mn.z};
    glm::vec3 v001 = {mn.x, mn.y, mx.z};
    glm::vec3 v010 = {mx.x, mn.y, mn.z};
    glm::vec3 v011 = {mx.x, mn.y, mx.z};
    glm::vec3 v100 = {mn.x, mx.y, mn.z};
    glm::vec3 v101 = {mn.x, mx.y, mx.z};
    glm::vec3 v110 = {mx.x, mx.y, mn.z};
    glm::vec3 v111 = {mx.x, mx.y, mx.z};

    addLine(v000, v010, color);
    addLine(v010, v011, color);
    addLine(v011, v001, color);
    addLine(v001, v000, color);
    addLine(v100, v110, color);
    addLine(v110, v111, color);
    addLine(v111, v101, color);
    addLine(v101, v100, color);
    addLine(v000, v100, color);
    addLine(v001, v101, color);
    addLine(v010, v110, color);
    addLine(v011, v111, color);
}

void DebugDraw::sphere(const BoundingSphere& s, const glm::vec3& color) {
    for (int i = 0; i < 3; ++i) {
        constexpr int segments = 48;
        glm::vec3 prev{};
        for (int k = 0; k <= segments; ++k) {
            constexpr glm::vec3 perp1s[3] = {{0, 1, 0}, {1, 0, 0}, {1, 0, 0}};
            constexpr glm::vec3 perp2s[3] = {{0, 0, 1}, {0, 0, 1}, {0, 1, 0}};
            float angle = glm::two_pi<float>() * static_cast<float>(k) / static_cast<float>(segments);
            glm::vec3 pt = s.center + (perp1s[i] * glm::cos(angle) + perp2s[i] * glm::sin(angle)) * s.radius;
            if (k > 0) addLine(prev, pt, color);
            prev = pt;
        }
    }
}

void DebugDraw::box(const glm::mat4& transform,
                    const glm::vec3& halfExtents,
                    const glm::vec3& center,
                    const glm::vec3& color) {
    const glm::vec3& c = center;
    const glm::vec3& h = halfExtents;
    glm::vec3 corners[8] = {
            {c.x - h.x, c.y - h.y, c.z - h.z},
            {c.x + h.x, c.y - h.y, c.z - h.z},
            {c.x + h.x, c.y + h.y, c.z - h.z},
            {c.x - h.x, c.y + h.y, c.z - h.z},
            {c.x - h.x, c.y - h.y, c.z + h.z},
            {c.x + h.x, c.y - h.y, c.z + h.z},
            {c.x + h.x, c.y + h.y, c.z + h.z},
            {c.x - h.x, c.y + h.y, c.z + h.z},
    };
    for (auto& v: corners)
        v = glm::vec3(transform * glm::vec4(v, 1.0f));

    addLine(corners[0], corners[1], color);
    addLine(corners[1], corners[2], color);
    addLine(corners[2], corners[3], color);
    addLine(corners[3], corners[0], color);
    addLine(corners[4], corners[5], color);
    addLine(corners[5], corners[6], color);
    addLine(corners[6], corners[7], color);
    addLine(corners[7], corners[4], color);
    addLine(corners[0], corners[4], color);
    addLine(corners[1], corners[5], color);
    addLine(corners[2], corners[6], color);
    addLine(corners[3], corners[7], color);
}

void DebugDraw::capsule(
        const glm::mat4& transform, float radius, float height, const glm::vec3& center, const glm::vec3& color) {
    glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(center, 1.0f));
    glm::vec3 up = glm::normalize(glm::vec3(transform * glm::vec4(0, 1, 0, 0)));
    glm::vec3 right = glm::normalize(glm::vec3(transform * glm::vec4(1, 0, 0, 0)));
    glm::vec3 fwd = glm::normalize(glm::vec3(transform * glm::vec4(0, 0, 1, 0)));

    float scaleY = glm::length(glm::vec3(transform[1]));
    float scaleXZ = glm::length(glm::vec3(transform[0]));
    float r = radius * scaleXZ;
    float halfH = height * 0.5f * scaleY;

    glm::vec3 topCenter = worldCenter + up * halfH;
    glm::vec3 botCenter = worldCenter - up * halfH;

    constexpr int segments = 32;

    auto addCircle = [&](const glm::vec3& c, const glm::vec3& p1, const glm::vec3& p2) {
        glm::vec3 prev{};
        for (int s = 0; s <= segments; ++s) {
            float angle = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
            glm::vec3 pt = c + (p1 * glm::cos(angle) + p2 * glm::sin(angle)) * r;
            if (s > 0) addLine(prev, pt, color);
            prev = pt;
        }
    };

    auto addSemicircle = [&](const glm::vec3& c, const glm::vec3& p1, const glm::vec3& axis, bool top) {
        glm::vec3 prev{};
        int half = segments / 2;
        for (int s = 0; s <= half; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(half);
            float angle = (top ? 0.0f : glm::pi<float>()) + t * glm::pi<float>();
            glm::vec3 pt = c + (p1 * glm::cos(angle) + axis * glm::sin(angle)) * r;
            if (s > 0) addLine(prev, pt, color);
            prev = pt;
        }
    };

    addCircle(topCenter, right, fwd);
    addCircle(botCenter, right, fwd);
    addSemicircle(topCenter, right, up, true);
    addSemicircle(botCenter, right, up, false);
    addSemicircle(topCenter, fwd, up, true);
    addSemicircle(botCenter, fwd, up, false);

    for (int i = 0; i < 4; ++i) {
        float angle = glm::half_pi<float>() * static_cast<float>(i);
        glm::vec3 offset = (right * glm::cos(angle) + fwd * glm::sin(angle)) * r;
        addLine(topCenter + offset, botCenter + offset, color);
    }
}

void DebugDraw::entityBoxCollider(EntityHandle id, const World& world, const glm::vec3& color) {
    const Actor* actor = world.getActor(id);
    if (!actor) return;
    const auto* transform = actor->getComponent<Transform>();
    const auto* boxCollider = actor->getComponent<BoxCollider>();
    if (!transform || !boxCollider) return;
    const glm::mat4 model = TransformUtils::resolveWorldMatrix(*transform, world);
    box(model, boxCollider->halfExtents, boxCollider->center, color);
}

void DebugDraw::entityCapsuleCollider(EntityHandle id, const World& world, const glm::vec3& color) {
    const Actor* actor = world.getActor(id);
    if (!actor) return;
    const auto* transform = actor->getComponent<Transform>();
    const auto* cap = actor->getComponent<CapsuleCollider>();
    if (!transform || !cap) return;
    const glm::mat4 model = TransformUtils::resolveWorldMatrix(*transform, world);
    capsule(model, cap->radius, cap->height, cap->center, color);
}

void DebugDraw::drawScene(const World& world) {
    if (flags_.boxColliders) {
        for (const auto& [entity, transform, collider]: world.query<Transform, BoxCollider>())
            box(TransformUtils::resolveWorldMatrix(*transform, world),
                collider->halfExtents,
                collider->center,
                {0.0f, 1.0f, 0.5f});
    }

    if (flags_.capsuleColliders) {
        for (const auto& [entity, transform, collider]: world.query<Transform, CapsuleCollider>())
            capsule(TransformUtils::resolveWorldMatrix(*transform, world),
                    collider->radius,
                    collider->height,
                    collider->center,
                    {0.0f, 0.8f, 1.0f});
    }

    if (flags_.bvh) {
        std::vector<Item> items;
        for (const auto& [entity, transform, collider]: world.query<Transform, BoxCollider>()) {
            const glm::mat4 model = TransformUtils::resolveWorldMatrix(*transform, world);
            const glm::vec3& c = collider->center;
            const glm::vec3& h = collider->halfExtents;
            items.emplace_back(AABB(std::vector<glm::vec3>{
                                       glm::vec3(model * glm::vec4(c.x - h.x, c.y - h.y, c.z - h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x + h.x, c.y - h.y, c.z - h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x + h.x, c.y + h.y, c.z - h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x - h.x, c.y + h.y, c.z - h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x - h.x, c.y - h.y, c.z + h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x + h.x, c.y - h.y, c.z + h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x + h.x, c.y + h.y, c.z + h.z, 1.0f)),
                                       glm::vec3(model * glm::vec4(c.x - h.x, c.y + h.y, c.z + h.z, 1.0f)),
                               }),
                               std::to_string(entity));
        }
        if (!items.empty()) {
            BVH bvh;
            bvh.build(std::move(items));
            for (const auto& node: bvh.nodes) {
                aabb(node.bounds, {1.0f, 1.0f, 0.0f});
                if (node.isLeaf()) {
                    for (uint32_t i = node.begin; i < node.begin + node.count; ++i)
                        aabb(bvh.items[i].aabb, {1.0f, 1.0f, 0.0f});
                }
            }
        }
    }
}
