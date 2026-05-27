#pragma once
#include "../rendering/EditorRenderer.hpp"
#include "../rendering/VulkanBackend.hpp"
#include "RuntimeContainer.hpp"

class EditorCamera;
class EditorSelection;
class EditorGizmos;
class PickingSystem;
class GizmoBuilder;
class ObjectTransformHandle;
class ImguiRoot;

class RenderingContainer {
public:
    RenderingContainer(GameWindow& window,
                       RuntimeContainer& runtime,
                       EditorCamera& editorCamera,
                       EditorSelection& editorSelection,
                       EditorGizmos& editorGizmos,
                       EntityManager& entityManager,
                       PickingSystem& pickingService,
                       GizmoBuilder& gizmosBuilder,
                       ObjectTransformHandle& objectTransformHandle,
                       ImguiRoot& imguiRoot) :

        vulkanBackend_(window,
                       runtime.vulkanContext(),
                       runtime.frameManager(),
                       runtime.renderTargetManager(),
                       runtime.geometryPass(),
                       runtime.lightingPass(),
                       runtime.gridPass(),
                       runtime.gizmoPass(),
                       runtime.objectIdPass(),
                       runtime.outlinePass(),
                       runtime.uiPass(),
                       runtime.swapchainManager(),
                       runtime.rendererSettings()),

        editorRenderer_(vulkanBackend_,
                        editorCamera,
                        editorSelection,
                        editorGizmos,
                        entityManager,
                        gizmosBuilder,
                        objectTransformHandle,
                        pickingService,
                        imguiRoot) {}

    EditorRenderer& editorRenderer() { return editorRenderer_; }

private:
    VulkanBackend vulkanBackend_;
    EditorRenderer editorRenderer_;
};
