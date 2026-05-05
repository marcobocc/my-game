#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include "../controller/EditorPickingSystem.hpp"

struct TransformDragState {
    GizmoType gizmoMode = GizmoType::Translation;

    struct TranslationDrag {
        std::string objectId;
        GizmoAxis axis;
        glm::vec3 axisDir;
        glm::vec3 dragPlaneNormal;
        glm::vec3 dragOrigin;
        glm::vec3 hitPointOnAxis;
    };
    std::optional<TranslationDrag> activeDrag;

    struct RotationDrag {
        std::string objectId;
        GizmoAxis axis;
        glm::vec3 axisDir;
        glm::vec3 origin;
        glm::quat initialRotation;
        glm::vec3 initialHitDir;
    };
    std::optional<RotationDrag> activeRotationDrag;

    struct ScaleDrag {
        std::string objectId;
        GizmoAxis axis;
        glm::vec3 axisDir;
        glm::vec3 origin;
        glm::vec3 initialScale;
        glm::vec3 dragPlaneNormal;
        float initialT;
    };
    std::optional<ScaleDrag> activeScaleDrag;

    void clearDrag() {
        activeDrag.reset();
        activeRotationDrag.reset();
        activeScaleDrag.reset();
    }

    bool isActiveDrag() const {
        return activeDrag.has_value() || activeRotationDrag.has_value() || activeScaleDrag.has_value();
    }
};
