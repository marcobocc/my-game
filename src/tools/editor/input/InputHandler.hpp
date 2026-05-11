#pragma once
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/modules/core/GameWindow.hpp"
#include "../business/UndoHistory.hpp"
#include "PickingSystem.hpp"

class EditorGizmos;
class EditorCamera;
class ObjectSelection;
class ObjectTransformHandle;
class Scene;
class SceneMutations;
class EditorSettings;

/*
    InputHandler

    Purpose:
    --------------------------------------------------
    Unified input handler for keyboard and mouse input.
    Invokes business/ directly without events.

    Responsibilities:
    --------------------------------------------------
    - Detect keyboard input and invoke service methods
    - Detect mouse input and invoke camera/gizmo service methods
    - Detect object picking and invoke selection service
    - Detect gizmo interactions and invoke gizmo service
*/
class InputHandler {
public:
    InputHandler(GameWindow& window,
                 GameEngine& engine,
                 PickingSystem& pickingSystem,
                 EditorCamera& editorCamera,
                 ObjectSelection& objectSelection,
                 ObjectTransformHandle& objectTransformHandle,
                 EntityManager& entityManager,
                 SceneMutations& sceneMutations,
                 EditorGizmos& editorGizmos,
                 EditorSettings& rendererSettings,
                 UndoHistory& undoHistory) :
        window_(window),
        engine_(engine),
        pickingSystem_(pickingSystem),
        editorCamera_(editorCamera),
        objectSelection_(objectSelection),
        objectTransformHandle_(objectTransformHandle),
        entityManager_(entityManager),
        sceneMutations_(sceneMutations),
        editorGizmos_(editorGizmos),
        rendererSettings_(rendererSettings),
        undoHistory_(undoHistory),
        wasLeftDown_(false) {}

    void update(double mouseX, double mouseY, double deltaTime);

private:
    GameWindow& window_;
    GameEngine& engine_;
    PickingSystem& pickingSystem_;
    EditorCamera& editorCamera_;
    ObjectSelection& objectSelection_;
    ObjectTransformHandle& objectTransformHandle_;
    EntityManager& entityManager_;
    SceneMutations& sceneMutations_;
    EditorGizmos& editorGizmos_;
    EditorSettings& rendererSettings_;
    UndoHistory& undoHistory_;
    bool wasLeftDown_;

    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool wasOrbiting_ = false;
    bool wasPanning_ = false;

    void handleKeyboardInput();
    void processGizmoDrag(double mouseX, double mouseY, bool leftDown);
    void processMouseInteraction(double mouseX, double mouseY, bool leftDown);
    void processCameraInput(double mouseX, double mouseY, double deltaTime);
};
