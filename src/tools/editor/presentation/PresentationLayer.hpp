#pragma once
#include <vector>
#include "../../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../engine/modules/ui/UserInterface.hpp"


class SceneLoader;
class PickingSystem;
class AssetManager;
class SceneMutations;
class VulkanEditorBackend;
class Scene;
class EditorOrbitCamera;
class GizmosRenderer;
class ObjectSelection;
class EditorGizmos;
class EditorSettings;
class ObjectTransformHandle;
class EventBus;
struct RendererSettings;
struct DrawCall;

/*
    PresentationLayer

    Purpose:
    --------------------------------------------------
    Contains all business logic for rendering operations.
    - Renders scenes according to EditorState
    - Builds gizmos (checks if object selected, draw transform gizmo, etc)
    - Handles object outlining based on selection
    - Handles render target creation/destruction and object outlining.
    - Instantiates UI panels

    Lighting settings are now managed by EditorSettings through events.
*/
class PresentationLayer {
public:
    PresentationLayer(VulkanEditorBackend& renderer,
                      EditorOrbitCamera& editorOrbitCamera,
                      ObjectSelection& objectSelection,
                      EditorGizmos& editorGizmos,
                      EditorSettings& editorSettings,
                      RendererSettings& rendererSettings,
                      Scene& scene,
                      GizmosRenderer& gizmosBuilder,
                      ObjectTransformHandle& objectTransformHandle,
                      UserInterface& userInterface,
                      PickingSystem& pickingService,
                      SceneMutations& sceneMutations,
                      SceneLoader& editorWorkspace,
                      AssetManager& assetManager);

    // --------------------------------------------------------
    // Rendering
    // --------------------------------------------------------
    void render(const Scene& scene, float gridScale, const std::vector<DrawCall>& outlineQueue);

private:
    void buildGizmos();
    void buildOutlines();

    VulkanEditorBackend& renderer_;
    EditorOrbitCamera& editorOrbitCamera_;
    ObjectSelection& objectSelection_;
    EditorGizmos& editorGizmos_;
    EditorSettings& editorSettings_;
    RendererSettings& rendererSettings_;
    Scene& scene_;
    GizmosRenderer& gizmosBuilder_;
    ObjectTransformHandle& objectTransformHandle_;
    UserInterface& userInterface_;
    PickingSystem& pickingSystem_;
    SceneMutations& sceneMutations_;
    SceneLoader& editorWorkspace_;
    AssetManager& assetManager_;
    std::vector<DrawCall> outlineQueue_;
    std::vector<VulkanGizmoPass::GizmoVertex> builtGizmoLines_;
};
