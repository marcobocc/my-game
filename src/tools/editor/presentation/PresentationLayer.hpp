#pragma once
#include <vector>
#include "../../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/modules/ui/UserInterface.hpp"


class SceneLoader;
class PickingSystem;
class AssetManager;
class SceneMutations;
class MaterialMutations;
class UndoHistory;
class VulkanEditorBackend;
class EditorCamera;
class GizmosRenderer;
class ObjectSelection;
class EditorGizmos;
class EditorSettings;
class ObjectTransformHandle;
class EventBus;
class GameEngine;
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
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("PresentationLayer");

public:
    PresentationLayer(VulkanEditorBackend& renderer,
                      EditorCamera& editorOrbitCamera,
                      ObjectSelection& objectSelection,
                      EditorGizmos& editorGizmos,
                      EditorSettings& editorSettings,
                      RendererSettings& rendererSettings,
                      EntityManager& entityManager,
                      GizmosRenderer& gizmosBuilder,
                      ObjectTransformHandle& objectTransformHandle,
                      UserInterface& userInterface,
                      PickingSystem& pickingService,
                      SceneMutations& sceneMutations,
                      MaterialMutations& materialMutations,
                      SceneLoader& editorWorkspace,
                      AssetManager& assetManager,
                      GameWindow& window,
                      GameEngine& engine,
                      UndoHistory& undoHistory);

    // --------------------------------------------------------
    // Rendering
    // --------------------------------------------------------
    void render(const EntityManager& entityManager, float gridScale, const std::vector<DrawCall>& outlineQueue);

private:
    void buildGizmos();
    void buildOutlines();

    VulkanEditorBackend& renderer_;
    EditorCamera& editorOrbitCamera_;
    ObjectSelection& objectSelection_;
    EditorGizmos& editorGizmos_;
    EditorSettings& editorSettings_;
    RendererSettings& rendererSettings_;
    EntityManager& entityManager_;
    GizmosRenderer& gizmosBuilder_;
    ObjectTransformHandle& objectTransformHandle_;
    UserInterface& userInterface_;
    PickingSystem& pickingSystem_;
    SceneMutations& sceneMutations_;
    SceneLoader& editorWorkspace_;
    AssetManager& assetManager_;
    GameEngine& engine_;
    GameWindow& window_;
    std::vector<DrawCall> outlineQueue_;
    std::vector<VulkanGizmoPass::GizmoVertex> builtGizmoLines_;
};
