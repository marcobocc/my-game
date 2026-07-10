#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "core/scene/EntityHandle.hpp"

enum class GizmoType { Translation, Rotation, Scale };
enum class GizmoAxis { X, Y, Z, All };

struct GizmoHit {
    std::vector<EntityHandle> objectIds;
    GizmoType type;
    GizmoAxis axis;
};

struct SceneObjectHit {
    EntityHandle objectId;
};

struct GizmoHandle {
    std::vector<EntityHandle> objectIds;
    GizmoType type;
    GizmoAxis axis;
    glm::vec3 base;
    glm::vec3 tip;
    float radius;
};
