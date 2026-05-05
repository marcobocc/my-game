#pragma once
#include <glm/glm.hpp>
#include <optional>
#include "../state/EditorState.hpp"
#include "../state/TransformDragState.hpp"

class GameWindow;
class GameEngine;
class EditorUIController;
class OrbitCameraController;

class TransformController {
public:
    TransformController(EditorState& editorState,
                        GameWindow& window,
                        GameEngine& engine,
                        EditorUIController& uiController,
                        OrbitCameraController& cameraController) :
        editorState_(editorState),
        window_(window),
        engine_(engine),
        uiController_(uiController),
        cameraController_(cameraController) {}

    void beginTranslationDrag(GizmoAxis axis, double mouseX, double mouseY);
    void updateTranslationDrag(double mouseX, double mouseY);
    void commitTranslationDrag();

    void beginRotationDrag(GizmoAxis axis, double mouseX, double mouseY);
    void updateRotationDrag(double mouseX, double mouseY);
    void commitRotationDrag();

    void beginScaleDrag(GizmoAxis axis, double mouseX, double mouseY);
    void updateScaleDrag(double mouseX, double mouseY);
    void commitScaleDrag();

    TransformDragState& getDragState() { return dragState_; }
    const TransformDragState& getDragState() const { return dragState_; }

private:
    EditorState& editorState_;
    GameWindow& window_;
    GameEngine& engine_;
    EditorUIController& uiController_;
    OrbitCameraController& cameraController_;
    TransformDragState dragState_;

    static std::optional<glm::vec3>
    rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal);

    static float projectOnAxis(const glm::vec3& point, const glm::vec3& axisOrigin, const glm::vec3& axisDir);

    Ray buildMouseRay(double mouseX, double mouseY) const;

    static glm::vec3 axisToDir(GizmoAxis axis);
};
