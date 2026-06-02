#pragma once

class UndoHistory;
class EditorCamera;
class EditorGizmos;
class EditorSettings;
class ObjectTransformHandle;
class SceneQuickActions;
class EditorContext;
class Imgui_Console;

enum class ActionID {
    UNKNOWN,

    UNDO,
    REDO,
    COPY,
    CUT,
    PASTE,
    DUPLICATE,
    DELETE,

    TOGGLE_GRID,
    TOGGLE_LIGHTING,
    TOGGLE_BVH,

    GIZMO_TRANSLATE,
    GIZMO_ROTATE,
    GIZMO_SCALE,

    CAMERA_RESET,

    INCREASE_SCALE,
    DECREASE_SCALE,
    TOGGLE_SNAPPING,

    SAVE,

    TOGGLE_CONSOLE
};

class ActionDispatcher {
public:
    ActionDispatcher(UndoHistory& undoHistory,
                     EditorCamera& editorCamera,
                     EditorGizmos& editorGizmos,
                     EditorSettings& editorSettings,
                     ObjectTransformHandle& objectTransformHandle,
                     SceneQuickActions& sceneQuickActions,
                     EditorContext& project,
                     Imgui_Console& console) :
        undoHistory_(undoHistory),
        editorCamera_(editorCamera),
        editorGizmos_(editorGizmos),
        editorSettings_(editorSettings),
        objectTransformHandle_(objectTransformHandle),
        sceneQuickActions_(sceneQuickActions),
        project_(project),
        console_(console) {}

    void execute(ActionID id);

private:
    UndoHistory& undoHistory_;
    EditorCamera& editorCamera_;
    EditorGizmos& editorGizmos_;
    EditorSettings& editorSettings_;
    ObjectTransformHandle& objectTransformHandle_;
    SceneQuickActions& sceneQuickActions_;
    EditorContext& project_;
    Imgui_Console& console_;
};
