#include "InputHandler.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "../../services/EditorSelection.hpp"
#include "../../services/EditorSettings.hpp"
#include "../../services/common_editing/RuntimeScene.hpp"
#include "../scene_viewport/editor_camera/EditorCamera.hpp"
#include "../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../scene_viewport/transform_handle/ObjectTransformHandle.hpp"


void InputHandler::update(double mouseX, double mouseY, double deltaTime, bool simActive) {
    bool imguiCapturingInput = ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureMouse;

    if (!ImGui::GetIO().WantTextInput) {
        handleKeyboardInput();
        bool leftDown = inputSystem_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        bool ctrl = false, shift = false, alt = false;
        getModifierState(ctrl, shift, alt);
        if (!terrainSculptMode_) {
            processGizmoDrag(mouseX, mouseY, leftDown);
            processMouseInteraction(mouseX, mouseY, leftDown, ctrl);
        }
        if (!simActive) processCameraInput(mouseX, mouseY, deltaTime);
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
                               GLFW_KEY_U,
                               GLFW_KEY_SLASH};

    for (int key: keysToCheck) {
        if (inputSystem_.isKeyPressed(key)) {
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
    ctrl = inputSystem_.isKeyDown(GLFW_KEY_LEFT_SUPER) || inputSystem_.isKeyDown(GLFW_KEY_RIGHT_SUPER);
#else
    ctrl = inputSystem_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || inputSystem_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
#endif
    shift = inputSystem_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || inputSystem_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
    alt = inputSystem_.isKeyDown(GLFW_KEY_LEFT_ALT) || inputSystem_.isKeyDown(GLFW_KEY_RIGHT_ALT);
    return true;
}

void InputHandler::processGizmoDrag(double mouseX, double mouseY, bool leftDown) {
    auto& dragState = objectTransformHandle_.getDragState();
    if (leftDown) {
        objectTransformHandle_.updateDrag(mouseX, mouseY);
    } else if (dragState.isActiveDrag()) {
        objectTransformHandle_.endDrag();
    }
}

void InputHandler::processMouseInteraction(double mouseX, double mouseY, bool leftDown, bool cmdDown) {
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
                if (cmdDown && editorSelection_.isEntitySelected(sceneHit->objectId)) {
                    editorSelection_.removeFromSelection(sceneHit->objectId);
                } else {
                    editorSelection_.addToSelection(sceneHit->objectId, cmdDown);
                }
            } else if (auto* gizmoHit = std::get_if<GizmoHit>(&*result)) {
                objectTransformHandle_.beginDrag(gizmoHit->type, gizmoHit->axis, mouseX, mouseY);
            }
        } else {
            editorSelection_.clearSelection();
        }
    }
}

void InputHandler::processCameraInput(double mouseX, double mouseY, double deltaTime) {
    bool rightDown = inputSystem_.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
    bool middleDown = inputSystem_.isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE);

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

    // Handle zoom — only when mouse is inside the scene viewport
    double scroll = inputSystem_.getScrollDelta();
    if (scroll != 0.0) {
        auto sv = window_.getSceneViewport();
        double mx = mouseX, my = mouseY;
        bool inViewport = mx >= sv.x && mx < sv.x + sv.width && my >= sv.y && my < sv.y + sv.height;
        if (inViewport) editorCamera_.zoom(scroll);
    }

    // Handle keyboard movement
    int moveDir = 0;
    if (inputSystem_.isKeyDown(GLFW_KEY_W)) moveDir |= 1;
    if (inputSystem_.isKeyDown(GLFW_KEY_S)) moveDir |= 2;
    if (inputSystem_.isKeyDown(GLFW_KEY_A)) moveDir |= 4;
    if (inputSystem_.isKeyDown(GLFW_KEY_D)) moveDir |= 8;

    if (moveDir != 0) {
        bool sprint = inputSystem_.isKeyDown(GLFW_KEY_LEFT_SHIFT);
        editorCamera_.moveFromKeyboard(moveDir, sprint, deltaTime);
    }

    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
}
