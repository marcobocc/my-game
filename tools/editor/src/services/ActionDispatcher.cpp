#include "ActionDispatcher.hpp"
#include "../features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "../features/scene_viewport/gizmos/EditorGizmos.hpp"
#include "../features/scene_viewport/transform_handle/ObjectTransformHandle.hpp"
#include "EditorContext.hpp"
#include "EditorSettings.hpp"
#include "UndoHistory.hpp"
#include "common_editing/SceneQuickActions.hpp"

template<>
struct std::hash<ActionID> {
    std::size_t operator()(const ActionID& id) const noexcept {
        return std::hash<std::underlying_type_t<ActionID>>{}(static_cast<std::underlying_type_t<ActionID>>(id));
    }
};

void ActionDispatcher::execute(ActionID id) {
    switch (id) {
        case ActionID::UNDO:
            undoHistory_.undo();
            break;
        case ActionID::REDO:
            undoHistory_.redo();
            break;
        case ActionID::COPY:
            sceneQuickActions_.copySelection();
            break;
        case ActionID::CUT:
            sceneQuickActions_.cutSelected();
            break;
        case ActionID::PASTE:
            sceneQuickActions_.pasteSelection();
            break;
        case ActionID::DUPLICATE:
            sceneQuickActions_.duplicateSelection();
            break;
        case ActionID::DELETE:
            sceneQuickActions_.deleteSelection();
            break;
        case ActionID::TOGGLE_GRID:
            editorSettings_.toggleGrid();
            break;
        case ActionID::TOGGLE_LIGHTING:
            editorSettings_.toggleLighting();
            break;
        case ActionID::TOGGLE_BVH:
            editorGizmos_.toggleBVH();
            break;
        case ActionID::GIZMO_TRANSLATE:
            objectTransformHandle_.getDragState().gizmoMode = GizmoType::Translation;
            break;
        case ActionID::GIZMO_ROTATE:
            objectTransformHandle_.getDragState().gizmoMode = GizmoType::Rotation;
            break;
        case ActionID::GIZMO_SCALE:
            objectTransformHandle_.getDragState().gizmoMode = GizmoType::Scale;
            break;
        case ActionID::CAMERA_RESET:
            editorCamera_.resetToDefault();
            break;
        case ActionID::INCREASE_SCALE:
            editorSettings_.increaseScale();
            break;
        case ActionID::DECREASE_SCALE:
            editorSettings_.decreaseScale();
            break;
        case ActionID::TOGGLE_SNAPPING:
            editorSettings_.toggleSnapping();
            break;
        case ActionID::SAVE:
            project_.saveCurrentScene();
            break;
        default:
            break;
    }
}
