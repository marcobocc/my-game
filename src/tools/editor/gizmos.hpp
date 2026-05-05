#pragma once

enum class GizmoType { Translation, Rotation, Scale };
enum class GizmoAxis { X, Y, Z, All };

struct GizmoHit {
    std::string objectId;
    GizmoType type;
    GizmoAxis axis;
};

struct SceneObjectHit {
    std::string objectId;
};

struct GizmoHandle {
    std::string objectId;
    GizmoType type;
    GizmoAxis axis;
    glm::vec3 base;
    glm::vec3 tip;
    float radius;
};
