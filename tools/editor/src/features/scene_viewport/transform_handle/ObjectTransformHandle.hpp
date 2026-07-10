#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include <vector>
#include "../../../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../../../runtime/src/graphics/components/Camera.hpp"
#include "../gizmos/gizmos.hpp"
#include "core/scene/EntityHandle.hpp"
#include "graphics/vulkan/passes/VulkanGizmoPass.hpp"
#include "math/Ray.hpp"

class GameWindow;
class RuntimeScene;
class EditorCamera;
class EditorSelection;
class EditorSettings;

struct ActiveDrag {
    GizmoType type;
    GizmoAxis axis;
    glm::vec3 pivot;
    glm::vec3 axisDir;
    glm::vec3 dragPlaneNormal;
    glm::vec3 initialHitPoint;
    glm::vec3 initialHitDir;
    float initialT;
};

struct TransformDragState {
    GizmoType gizmoMode = GizmoType::Translation;
    std::optional<ActiveDrag> activeDrag;

    void clearDrag() { activeDrag.reset(); }
    bool isActiveDrag() const { return activeDrag.has_value(); }
};

class ObjectTransformHandle {
public:
    ObjectTransformHandle(GameWindow& window,
                          RuntimeScene& scene,
                          EditorCamera& editorOrbitCamera,
                          EditorSelection& editorSelection,
                          EditorSettings& rendererSettings) :
        window_(window),
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

    glm::vec3 computeGroupPivot(const std::vector<EntityHandle>& ids) const;

    using GizmoObject = std::vector<VulkanGizmoPass::GizmoVertex>;

    struct GizmoTransformHandle {
        GizmoObject visualization;
        std::vector<GizmoHandle> pickingHandles;
    };

    GizmoTransformHandle buildTranslationGizmo(glm::vec3 pivot, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle buildRotationGizmo(glm::vec3 pivot, const Camera& camera, const Transform& cameraTransform);
    GizmoTransformHandle buildScaleGizmo(glm::vec3 pivot, const Camera& camera, const Transform& cameraTransform);

private:
    GameWindow& window_;
    RuntimeScene& scene_;
    EditorCamera& editorOrbitCamera_;
    EditorSelection& editorSelection_;
    EditorSettings& rendererSettings_;
    TransformDragState dragState_;
    std::string dragGroupId_;

    glm::vec3 prevTranslationDelta_{0.0f};
    float prevRotationAngle_ = 0.0f;
    float prevScaleFactor_ = 1.0f;

    static std::optional<glm::vec3>
    rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal);

    static float projectOnAxis(const glm::vec3& point, const glm::vec3& axisOrigin, const glm::vec3& axisDir);

    Ray buildMouseRay(double mouseX, double mouseY) const;

    static glm::vec3 axisToDir(GizmoAxis axis);

    glm::vec3 transformAxisToLocalSpace(GizmoAxis axis, const Transform& transform);

    void addGizmoLine(GizmoObject& gizmoObject, const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    GizmoObject buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color);
};
