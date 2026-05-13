#include "ActionDispatcher.hpp"
#include "EditorCamera.hpp"
#include "EditorGizmos.hpp"
#include "EditorSettings.hpp"
#include "ObjectTransformHandle.hpp"
#include "SceneLoader.hpp"
#include "UndoHistory.hpp"
#include "scene_editing/SceneMutations.hpp"

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
            sceneMutations_.copySelectedObject();
            break;
        case ActionID::CUT:
            sceneMutations_.cutSelectedObject();
            break;
        case ActionID::PASTE:
            sceneMutations_.pasteObject();
            break;
        case ActionID::DUPLICATE:
            sceneMutations_.duplicateSelectedObject();
            break;
        case ActionID::DELETE:
            sceneMutations_.deleteSelectedObject();
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
            sceneLoader_.saveCurrentScene();
            break;
        default:
            break;
    }
}
