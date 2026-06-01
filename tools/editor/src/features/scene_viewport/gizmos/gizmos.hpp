#pragma once
#include "modules/scene/EntityHandle.hpp"

enum class GizmoType { Translation, Rotation, Scale };
enum class GizmoAxis { X, Y, Z, All };

struct GizmoHit {
    EntityHandle objectId;
    GizmoType type;
    GizmoAxis axis;
};

struct SceneObjectHit {
    EntityHandle objectId;
};

struct GizmoHandle {
    EntityHandle objectId;
    GizmoType type;
    GizmoAxis axis;
    glm::vec3 base;
    glm::vec3 tip;
    float radius;
};
