#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include <vector>
#include "../gizmos/gizmos.hpp"
#include "modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"
#include "utils/math/Ray.hpp"

class GameWindow;
class GameEngine;
class RuntimeScene;
class EditorCamera;
class EditorSelection;
class EditorSettings;

struct TransformDragState {
    GizmoType gizmoMode = GizmoType::Translation;

    struct TranslationDrag {
        EntityHandle objectId;
        GizmoAxis axis;
        glm::vec3 axisDir;
        glm::vec3 dragPlaneNormal;
        glm::vec3 dragOrigin;
        glm::vec3 hitPointOnAxis;
    };
    std::optional<TranslationDrag> activeDrag;

    struct RotationDrag {
        EntityHandle objectId;
        GizmoAxis axis;
        glm::vec3 axisDir;
        glm::vec3 origin;
        glm::quat initialRotation;
        glm::vec3 initialHitDir;
    };
    std::optional<RotationDrag> activeRotationDrag;

    struct ScaleDrag {
        EntityHandle objectId;
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

class ObjectTransformHandle {
public:
    ObjectTransformHandle(GameWindow& window,
                          GameEngine& engine,
                          RuntimeScene& scene,
                          EditorCamera& editorOrbitCamera,
                          EditorSelection& editorSelection,
                          EditorSettings& rendererSettings) :
        window_(window),
        engine_(engine),
        scene_(scene),
        editorOrbitCamera_(editorOrbitCamera),
        editorSelection_(editorSelection),
        rendererSettings_(rendererSettings) {}

    void beginTranslationDrag(GizmoAxis axis, double mouseX, double mouseY);
    void updateTranslationDrag(double mouseX, double mouseY);
    void commitTranslationDrag();

    void beginRotationDrag(GizmoAxis axis, double mouseX, double mouseY);
    void updateRotationDrag(double mouseX, double mouseY);
    void commitRotationDrag();

    void beginScaleDrag(GizmoAxis axis, double mouseX, double mouseY);
    void updateScaleDrag(double mouseX, double mouseY);
    void commitScaleDrag();

    void beginDrag(GizmoType type, GizmoAxis axis, double mouseX, double mouseY);
    void updateDrag(double mouseX, double mouseY);
    void endDrag();

    TransformDragState& getDragState() { return dragState_; }
    const TransformDragState& getDragState() const { return dragState_; }

    using GizmoObject = std::vector<VulkanGizmoPass::GizmoVertex>;

    struct GizmoTransformHandle {
        GizmoObject visualization;
        std::vector<GizmoHandle> pickingHandles;
    };

    GizmoTransformHandle
    buildTranslationGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle
    buildRotationGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle buildScaleGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform);

private:
    GameWindow& window_;
    GameEngine& engine_;
    RuntimeScene& scene_;
    EditorCamera& editorOrbitCamera_;
    EditorSelection& editorSelection_;
    EditorSettings& rendererSettings_;
    TransformDragState dragState_;
    std::string dragGroupId_;

    static std::optional<glm::vec3>
    rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal);

    static float projectOnAxis(const glm::vec3& point, const glm::vec3& axisOrigin, const glm::vec3& axisDir);

    Ray buildMouseRay(double mouseX, double mouseY) const;

    static glm::vec3 axisToDir(GizmoAxis axis);

    glm::vec3 transformAxisToLocalSpace(GizmoAxis axis, const Transform& transform);

    void addGizmoLine(GizmoObject& gizmoObject, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    GizmoObject buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color);
};
