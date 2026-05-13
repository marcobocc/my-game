#pragma once

class UndoHistory;
class EditorCamera;
class EditorGizmos;
class EditorSettings;
class ObjectTransformHandle;
class SceneMutations;
class SceneLoader;

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

    SAVE
};

class ActionDispatcher {
public:
    ActionDispatcher(UndoHistory& undoHistory,
                     EditorCamera& editorCamera,
                     EditorGizmos& editorGizmos,
                     EditorSettings& editorSettings,
                     ObjectTransformHandle& objectTransformHandle,
                     SceneMutations& sceneMutations,
                     SceneLoader& sceneLoader) :
        undoHistory_(undoHistory),
        editorCamera_(editorCamera),
        editorGizmos_(editorGizmos),
        editorSettings_(editorSettings),
        objectTransformHandle_(objectTransformHandle),
        sceneMutations_(sceneMutations),
        sceneLoader_(sceneLoader) {}

    void execute(ActionID id);

private:
    UndoHistory& undoHistory_;
    EditorCamera& editorCamera_;
    EditorGizmos& editorGizmos_;
    EditorSettings& editorSettings_;
    ObjectTransformHandle& objectTransformHandle_;
    SceneMutations& sceneMutations_;
    SceneLoader& sceneLoader_;
};
