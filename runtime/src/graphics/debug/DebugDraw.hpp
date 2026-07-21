#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../../math/AABB.hpp"
#include "../../math/BoundingSphere.hpp"
#include "../../physics/ConvexHull.hpp"
#include "core/scene/EntityHandle.hpp"
#include "graphics/vulkan/passes/VulkanGizmoPass.hpp"

class World;

class DebugDraw {
public:
    struct Flags {
        bool boxColliders = false;
        bool capsuleColliders = false;
        bool bvh = false;
    };

    Flags& flags() { return flags_; }
    const Flags& flags() const { return flags_; }
    void drawScene(const World& world);

    void flush() { lines_.clear(); }

    void line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color = {0.0f, 1.0f, 0.0f});
    void aabb(const AABB& box, const glm::vec3& color = {0.0f, 1.0f, 0.0f});
    void sphere(const BoundingSphere& s, const glm::vec3& color = {1.0f, 1.0f, 0.0f});
    void box(const glm::mat4& transform,
             const glm::vec3& halfExtents,
             const glm::vec3& center,
             const glm::vec3& color = {0.0f, 1.0f, 0.5f});
    void capsule(const glm::mat4& transform,
                 float radius,
                 float height,
                 const glm::vec3& center,
                 const glm::vec3& color = {0.0f, 0.8f, 1.0f});
    void hull(const glm::mat4& transform, const ConvexHull& hull, const glm::vec3& color = {0.9f, 0.4f, 0.9f});

    void entityBoxCollider(EntityHandle id, const World& world, const glm::vec3& color = {0.0f, 1.0f, 0.5f});
    void entityCapsuleCollider(EntityHandle id, const World& world, const glm::vec3& color = {0.0f, 0.8f, 1.0f});
    const std::vector<VulkanGizmoPass::GizmoVertex>& getVertices() const { return lines_; }

private:
    void addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

    Flags flags_;
    std::vector<VulkanGizmoPass::GizmoVertex> lines_;
};
