#pragma once
#include "../rendering/EditorRenderer.hpp"
#include "RuntimeContainer.hpp"

class EditorCamera;
class EditorSelection;
class EditorGizmos;
class PickingSystem;
class GizmoBuilder;
class ObjectTransformHandle;
class ImguiRoot;
class SimHUDRoot;
class WelcomeRoot;

class RenderingContainer {
public:
    RenderingContainer(GameWindow& window,
                       RuntimeContainer& runtime,
                       EditorCamera& editorCamera,
                       EditorSelection& editorSelection,
                       EditorGizmos& editorGizmos,
                       World& entityManager,
                       PickingSystem& pickingService,
                       GizmoBuilder& gizmosBuilder,
                       ObjectTransformHandle& objectTransformHandle,
                       ImguiRoot& imguiRoot,
                       SimHUDRoot& simHUDRoot,
                       WelcomeRoot& welcomeRoot) :

        editorRenderer_(runtime.vulkanBackend(),
                        editorCamera,
                        editorSelection,
                        editorGizmos,
                        entityManager,
                        gizmosBuilder,
                        objectTransformHandle,
                        pickingService,
                        imguiRoot,
                        simHUDRoot,
                        welcomeRoot) {}

    EditorRenderer& editorRenderer() { return editorRenderer_; }

private:
    EditorRenderer editorRenderer_;
};
