#include "ShortcutController.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include "../controller/EditorUIController.hpp"
#include "../controller/RenderingController.hpp"
#include "../controller/TransformController.hpp"
#include "../state/EditorState.hpp"

void ShortcutController::update() {
    handleSaveAndUndoRedo();
    handleToggleKeys();
    handleGizmoModeKeys();
    handleGridRescale();
    handleGridSnapping();
}

void ShortcutController::handleSaveAndUndoRedo() {
    bool ctrlHeld = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
    bool shiftHeld = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || engine_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);

    if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_S) && uiController_.scenePath)
        uiController_.saveScene(*uiController_.scenePath);

    if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_Z)) {
        shiftHeld ? uiController_.mutations.undoHistory().redo() : uiController_.mutations.undoHistory().undo();
    }
}

void ShortcutController::handleToggleKeys() {
    if (engine_.isKeyPressed(GLFW_KEY_G)) renderingController_.toggleWorldGrid();
    if (engine_.isKeyPressed(GLFW_KEY_L)) renderingController_.toggleLighting();
    if (engine_.isKeyPressed(GLFW_KEY_B)) editorState_.toggleBVH();
}

void ShortcutController::handleGizmoModeKeys() {
    auto& dragState = transformController_.getDragState();
    if (engine_.isKeyPressed(GLFW_KEY_T)) dragState.gizmoMode = GizmoType::Translation;
    if (engine_.isKeyPressed(GLFW_KEY_R)) dragState.gizmoMode = GizmoType::Rotation;
    if (engine_.isKeyPressed(GLFW_KEY_Y)) dragState.gizmoMode = GizmoType::Scale;
}

void ShortcutController::handleGridRescale() {
    static constexpr float rescaleFactor = 1.5f;
    static const float upperBound = pow(rescaleFactor, 3);
    static const float lowerBound = 1 / pow(rescaleFactor, 14);
    if (engine_.isKeyPressed(GLFW_KEY_KP_ADD)) {
        float newScale = editorState_.getGridScale() * rescaleFactor;
        editorState_.setGridScale(std::clamp(newScale, lowerBound, upperBound));
    }
    if (engine_.isKeyPressed(GLFW_KEY_KP_SUBTRACT)) {
        float newScale = editorState_.getGridScale() / rescaleFactor;
        editorState_.setGridScale(std::clamp(newScale, lowerBound, upperBound));
    }
}

void ShortcutController::handleGridSnapping() {
    if (engine_.isKeyPressed(GLFW_KEY_U)) {
        editorState_.toggleGridSnapping();
    }
}
