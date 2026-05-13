#include "InputHandler.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "../../../engine/GameEngine.hpp"
#include "../business/EditorCamera.hpp"
#include "../business/EditorGizmos.hpp"
#include "../business/EditorSettings.hpp"
#include "../business/ObjectSelection.hpp"
#include "../business/ObjectTransformHandle.hpp"
#include "../business/scene_editing/SceneMutations.hpp"

void InputHandler::update(double mouseX, double mouseY, double deltaTime) {
    bool imguiCapturingInput = ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureMouse;

    if (!imguiCapturingInput) {
        handleKeyboardInput();
        bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        processGizmoDrag(mouseX, mouseY, leftDown);
        processMouseInteraction(mouseX, mouseY, leftDown);
        processCameraInput(mouseX, mouseY, deltaTime);
        wasLeftDown_ = leftDown;
    } else {
        wasLeftDown_ = false;
    }
}

void InputHandler::handleKeyboardInput() {

    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    getModifierState(ctrl, shift, alt);

    // Check for all possible keys that might have shortcut bindings
    const int keysToCheck[] = {GLFW_KEY_Z,
                               GLFW_KEY_C,
                               GLFW_KEY_X,
                               GLFW_KEY_V,
                               GLFW_KEY_D,
                               GLFW_KEY_G,
                               GLFW_KEY_L,
                               GLFW_KEY_B,
                               GLFW_KEY_T,
                               GLFW_KEY_R,
                               GLFW_KEY_Y,
                               GLFW_KEY_F,
                               GLFW_KEY_KP_ADD,
                               GLFW_KEY_KP_SUBTRACT,
                               GLFW_KEY_U};

    for (int key: keysToCheck) {
        if (engine_.isKeyPressed(key)) {
            Shortcut shortcut{ctrl, shift, alt, key};
            auto action = shortcutBindingService_.getAction(shortcut);
            if (action) {
                actionDispatcher_.execute(*action);
            }
        }
    }
}

bool InputHandler::matchesShortcut(const Shortcut& s, bool ctrl, bool shift, bool alt, int key) const {
    return s.ctrl == ctrl && s.shift == shift && s.alt == alt && s.key == key;
}

bool InputHandler::getModifierState(bool& ctrl, bool& shift, bool& alt) const {
#ifdef __APPLE__
    ctrl = engine_.isKeyDown(GLFW_KEY_LEFT_SUPER) || engine_.isKeyDown(GLFW_KEY_RIGHT_SUPER);
#else
    ctrl = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
#endif
    shift = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || engine_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
    alt = engine_.isKeyDown(GLFW_KEY_LEFT_ALT) || engine_.isKeyDown(GLFW_KEY_RIGHT_ALT);
    return true;
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
                                          editorCamera_.getCamera(),
                                          editorCamera_.getCameraTransform(),
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
            if (dx != 0.0f || dy != 0.0f) {
                editorCamera_.orbit(dx, dy);
            }
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
            editorCamera_.pan(dx, dy);
        }
        wasPanning_ = true;
    } else {
        wasPanning_ = false;
    }

    // Handle zoom
    double scroll = engine_.getScrollDelta();
    if (scroll != 0.0) {
        editorCamera_.zoom(scroll);
    }

    // Handle keyboard movement
    int moveDir = 0;
    if (engine_.isKeyDown(GLFW_KEY_W)) moveDir |= 1;
    if (engine_.isKeyDown(GLFW_KEY_S)) moveDir |= 2;
    if (engine_.isKeyDown(GLFW_KEY_A)) moveDir |= 4;
    if (engine_.isKeyDown(GLFW_KEY_D)) moveDir |= 8;

    if (moveDir != 0) {
        bool sprint = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT);
        editorCamera_.moveFromKeyboard(moveDir, sprint, deltaTime);
    }

    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
}
