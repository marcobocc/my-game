#pragma once
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "EditorPickingSystem.hpp"

class GameWindow;
class GameEngine;
class EditorState;
class TransformController;

/*
    InteractionController

    Purpose:
    --------------------------------------------------
    Manages all mouse-based interactions in the editor viewport.

    Responsibilities:
    --------------------------------------------------
    - Mouse picking and object selection
    - Gizmo drag operations (translate, rotate, scale)
    - Click detection and hit testing
*/
class InteractionController {
public:
    InteractionController(GameWindow& window,
                          GameEngine& engine,
                          EditorState& editorState,
                          EditorPickingSystem& pickingSystem,
                          TransformController& transformController) :
        window_(window),
        engine_(engine),
        editorState_(editorState),
        pickingSystem_(pickingSystem),
        transformController_(transformController),
        wasLeftDown_(false) {}

    // --------------------------------------------------------
    // Main Update
    // --------------------------------------------------------
    void update(double mouseX, double mouseY, const Camera& camera, const Transform& cameraTransform);

    // --------------------------------------------------------
    // State Tracking
    // --------------------------------------------------------
    bool wasLeftMouseDown() const { return wasLeftDown_; }

private:
    GameWindow& window_;
    GameEngine& engine_;
    EditorState& editorState_;
    EditorPickingSystem& pickingSystem_;
    TransformController& transformController_;
    bool wasLeftDown_;

    // --------------------------------------------------------
    // Interaction Handlers
    // --------------------------------------------------------
    void processGizmoDrag(double mouseX, double mouseY, bool leftDown);
    void processMouseInteraction(
            double mouseX, double mouseY, bool leftDown, const Camera& camera, const Transform& cameraTransform);

    void onPickResult(const SceneObjectHit& hit);
    void onPickResult(const GizmoHit& hit, double mouseX, double mouseY);
};
