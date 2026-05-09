#include "InputHandler.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "../../../engine/GameEngine.hpp"
#include "../business/EditorGizmos.hpp"
#include "../business/EditorOrbitCamera.hpp"
#include "../business/EditorSettings.hpp"
#include "../business/ObjectSelection.hpp"
#include "../business/ObjectTransformHandle.hpp"
#include "../business/scene_editing/SceneMutations.hpp"

void InputHandler::update(double mouseX, double mouseY, double deltaTime) {
    handleKeyboardInput();

    bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
    processGizmoDrag(mouseX, mouseY, leftDown);
    processMouseInteraction(mouseX, mouseY, leftDown);
    processCameraInput(mouseX, mouseY, deltaTime);
    wasLeftDown_ = leftDown;
}

void InputHandler::handleKeyboardInput() {
    bool ctrl = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
    bool shift = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || engine_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);

    // Undo / Redo
    if (ctrl && engine_.isKeyPressed(GLFW_KEY_Z)) {
        if (shift)
            sceneMutations_.redo();
        else
            sceneMutations_.undo();
    }

    // Toggles
    if (engine_.isKeyPressed(GLFW_KEY_G)) {
        rendererSettings_.toggleGrid();
    }

    if (engine_.isKeyPressed(GLFW_KEY_L)) {
        rendererSettings_.toggleLighting();
    }

    if (engine_.isKeyPressed(GLFW_KEY_B)) {
        editorGizmos_.toggleBVH();
    }

    // Gizmo modes
    if (engine_.isKeyPressed(GLFW_KEY_T)) {
        objectTransformHandle_.getDragState().gizmoMode = GizmoType::Translation;
    }

    if (engine_.isKeyPressed(GLFW_KEY_R)) {
        objectTransformHandle_.getDragState().gizmoMode = GizmoType::Rotation;
    }

    if (engine_.isKeyPressed(GLFW_KEY_Y)) {
        objectTransformHandle_.getDragState().gizmoMode = GizmoType::Scale;
    }

    // Grid scale
    if (engine_.isKeyPressed(GLFW_KEY_KP_ADD)) {
        rendererSettings_.increaseScale();
    }

    if (engine_.isKeyPressed(GLFW_KEY_KP_SUBTRACT)) {
        rendererSettings_.decreaseScale();
    }

    // Grid snapping
    if (engine_.isKeyPressed(GLFW_KEY_U)) {
        rendererSettings_.toggleSnapping();
    }

    // Camera reset
    if (engine_.isKeyPressed(GLFW_KEY_F)) {
        editorOrbitCamera_.resetToDefault();
    }

    // Object sceneMutations
    if (shift && engine_.isKeyPressed(GLFW_KEY_X)) {
        auto selectedId = objectSelection_.getSelectedEntityId();
        if (selectedId.has_value()) sceneMutations_.destroyObject(*selectedId);
    }
}

void InputHandler::processGizmoDrag(double mouseX, double mouseY, bool leftDown) {
    auto& dragState = objectTransformHandle_.getDragState();
    if (leftDown) {
        objectTransformHandle_.updateDrag(mouseX, mouseY);
    } else if (dragState.activeDrag || dragState.activeRotationDrag || dragState.activeScaleDrag) {
        objectTransformHandle_.endDrag();
    }
}

void InputHandler::processMouseInteraction(double mouseX, double mouseY, bool leftDown) {
    if (leftDown && !wasLeftDown_ && !ImGui::GetIO().WantCaptureMouse) {
        auto sv = window_.getSceneViewport();
        auto result = pickingSystem_.pick(static_cast<uint32_t>(mouseX),
                                          static_cast<uint32_t>(mouseY),
                                          static_cast<uint32_t>(sv.x),
                                          static_cast<uint32_t>(sv.y),
                                          static_cast<uint32_t>(sv.width),
                                          static_cast<uint32_t>(sv.height),
                                          editorOrbitCamera_.getCamera(),
                                          editorOrbitCamera_.getCameraTransform(),
                                          entityManager_);
        if (result) {
            if (auto* sceneHit = std::get_if<SceneObjectHit>(&*result)) {
                objectSelection_.selectObject(sceneHit->objectId);
            } else if (auto* gizmoHit = std::get_if<GizmoHit>(&*result)) {
                objectTransformHandle_.beginDrag(gizmoHit->type, gizmoHit->axis, mouseX, mouseY);
            }
        } else {
            objectSelection_.clearSelection();
        }
    }
}

void InputHandler::processCameraInput(double mouseX, double mouseY, double deltaTime) {
    bool rightDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
    bool middleDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE);

    // Handle orbit
    if (rightDown) {
        if (wasOrbiting_) {
            float dx = static_cast<float>(mouseX - lastMouseX_);
            float dy = static_cast<float>(mouseY - lastMouseY_);
            editorOrbitCamera_.orbit(dx, dy);
        }
        wasOrbiting_ = true;
    } else {
        wasOrbiting_ = false;
    }

    // Handle pan
    if (middleDown) {
        if (wasPanning_) {
            float dx = static_cast<float>(mouseX - lastMouseX_);
            float dy = static_cast<float>(mouseY - lastMouseY_);
            editorOrbitCamera_.pan(dx, dy);
        }
        wasPanning_ = true;
    } else {
        wasPanning_ = false;
    }

    // Handle zoom
    double scroll = engine_.getScrollDelta();
    if (scroll != 0.0) {
        editorOrbitCamera_.zoom(scroll);
    }

    // Handle keyboard movement
    int moveDir = 0;
    if (engine_.isKeyDown(GLFW_KEY_W)) moveDir |= 1;
    if (engine_.isKeyDown(GLFW_KEY_S)) moveDir |= 2;
    if (engine_.isKeyDown(GLFW_KEY_A)) moveDir |= 4;
    if (engine_.isKeyDown(GLFW_KEY_D)) moveDir |= 8;

    if (moveDir != 0) {
        bool sprint = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT);
        editorOrbitCamera_.moveFromKeyboard(moveDir, sprint, deltaTime);
    }

    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
}
