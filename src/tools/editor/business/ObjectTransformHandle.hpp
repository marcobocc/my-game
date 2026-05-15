#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/modules/scene/EntityMetadata.hpp"
#include "../../../engine/utils/math/Ray.hpp"
#include "../gizmos.hpp"

class GameWindow;
class GameEngine;
class SceneMutations;
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
                          SceneMutations& sceneMutations,
                          EditorCamera& editorOrbitCamera,
                          EditorSelection& editorSelection,
                          EditorSettings& rendererSettings,
                          EntityManager& entityManager) :
        window_(window),
        engine_(engine),
        sceneMutations_(sceneMutations),
        editorOrbitCamera_(editorOrbitCamera),
        editorSelection_(editorSelection),
        rendererSettings_(rendererSettings),
        entityManager_(entityManager) {}

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

private:
    GameWindow& window_;
    GameEngine& engine_;
    SceneMutations& sceneMutations_;
    EditorCamera& editorOrbitCamera_;
    EditorSelection& editorSelection_;
    EditorSettings& rendererSettings_;
    EntityManager& entityManager_;
    TransformDragState dragState_;

    static std::optional<glm::vec3>
    rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal);

    static float projectOnAxis(const glm::vec3& point, const glm::vec3& axisOrigin, const glm::vec3& axisDir);

    Ray buildMouseRay(double mouseX, double mouseY) const;

    static glm::vec3 axisToDir(GizmoAxis axis);

    glm::vec3 transformAxisToLocalSpace(GizmoAxis axis, const Transform& transform);
};
