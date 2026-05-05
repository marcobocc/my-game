#include "InteractionController.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/systems/core/GameWindow.hpp"
#include "../controller/TransformController.hpp"
#include "../state/EditorState.hpp"

void InteractionController::update(double mouseX,
                                   double mouseY,
                                   const Camera& camera,
                                   const Transform& cameraTransform) {
    bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
    processGizmoDrag(mouseX, mouseY, leftDown);
    processMouseInteraction(mouseX, mouseY, leftDown, camera, cameraTransform);
    wasLeftDown_ = leftDown;
}

void InteractionController::processGizmoDrag(double mouseX, double mouseY, bool leftDown) {
    auto& dragState = transformController_.getDragState();
    if (dragState.activeDrag) {
        if (leftDown) {
            transformController_.updateTranslationDrag(mouseX, mouseY);
        } else {
            transformController_.commitTranslationDrag();
        }
    } else if (dragState.activeRotationDrag) {
        if (leftDown) {
            transformController_.updateRotationDrag(mouseX, mouseY);
        } else {
            transformController_.commitRotationDrag();
        }
    } else if (dragState.activeScaleDrag) {
        if (leftDown) {
            transformController_.updateScaleDrag(mouseX, mouseY);
        } else {
            transformController_.commitScaleDrag();
        }
    }
}

void InteractionController::processMouseInteraction(
        double mouseX, double mouseY, bool leftDown, const Camera& camera, const Transform& cameraTransform) {
    if (leftDown && !wasLeftDown_ && !ImGui::GetIO().WantCaptureMouse) {
        auto sv = window_.getSceneViewport();
        auto result = pickingSystem_.pick(static_cast<uint32_t>(mouseX),
                                          static_cast<uint32_t>(mouseY),
                                          static_cast<uint32_t>(sv.x),
                                          static_cast<uint32_t>(sv.y),
                                          static_cast<uint32_t>(sv.width),
                                          static_cast<uint32_t>(sv.height),
                                          camera,
                                          cameraTransform,
                                          editorState_.getScene());
        if (result) {
            if (auto* sceneHit = std::get_if<SceneObjectHit>(&*result)) {
                onPickResult(*sceneHit);
            } else if (auto* gizmoHit = std::get_if<GizmoHit>(&*result)) {
                onPickResult(*gizmoHit, mouseX, mouseY);
            }
        } else {
            editorState_.setSelectedObject({});
        }
    }
}

void InteractionController::onPickResult(const SceneObjectHit& hit) {
    auto current = editorState_.getSelectedObject();
    if (current == hit.objectId)
        editorState_.setSelectedObject({});
    else
        editorState_.setSelectedObject(hit.objectId);
}

void InteractionController::onPickResult(const GizmoHit& hit, double mouseX, double mouseY) {
    if (hit.type == GizmoType::Translation) {
        transformController_.beginTranslationDrag(hit.axis, mouseX, mouseY);
    } else if (hit.type == GizmoType::Rotation) {
        transformController_.beginRotationDrag(hit.axis, mouseX, mouseY);
    } else if (hit.type == GizmoType::Scale) {
        transformController_.beginScaleDrag(hit.axis, mouseX, mouseY);
    }
}
