#pragma once
#include "../../../engine/GameEngine.hpp"

class EditorUIController;
class RenderingController;
class EditorState;
class TransformController;

/*
    ShortcutController

    Purpose:
    --------------------------------------------------
    Manages all keyboard shortcuts and hotkeys for the editor.

    Responsibilities:
    --------------------------------------------------
    - File operations (Save: Ctrl+S)
    - Undo/Redo (Ctrl+Z, Ctrl+Shift+Z)
    - View toggles (G: Grid, L: Lighting, B: BVH visualization)
    - Gizmo mode switching (T: Translate, R: Rotate, Y: Scale)
*/
class ShortcutController {
public:
    ShortcutController(GameEngine& engine,
                       EditorUIController& uiController,
                       RenderingController& renderingController,
                       EditorState& editorState,
                       TransformController& transformController) :
        engine_(engine),
        uiController_(uiController),
        renderingController_(renderingController),
        editorState_(editorState),
        transformController_(transformController) {}

    // --------------------------------------------------------
    // Main Update
    // --------------------------------------------------------
    void update();

private:
    GameEngine& engine_;
    EditorUIController& uiController_;
    RenderingController& renderingController_;
    EditorState& editorState_;
    TransformController& transformController_;

    // --------------------------------------------------------
    // Shortcut Handlers
    // --------------------------------------------------------
    void handleSaveAndUndoRedo();
    void handleToggleKeys();
    void handleGizmoModeKeys();
};
