#pragma once
#include <vector>
#include "../../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/modules/ui/UserInterface.hpp"


class SceneLoader;
class PickingSystem;
class EditorAssetRepository;
class SceneMutations;
class MaterialMutations;
class UndoHistory;
class VulkanEditorBackend;
class EditorCamera;
class GizmosRenderer;
class EditorSelection;
class ObjectBuilder;
class EditorGizmos;
class EditorSettings;
class ObjectTransformHandle;
class ActionDispatcher;
class ShortcutBindingService;
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
                      EditorSelection& editorSelection,
                      EditorGizmos& editorGizmos,
                      EditorSettings& editorSettings,
                      RendererSettings& rendererSettings,
                      EntityManager& entityManager,
                      GizmosRenderer& gizmosBuilder,
                      ObjectTransformHandle& objectTransformHandle,
                      UserInterface& userInterface,
                      PickingSystem& pickingService,
                      SceneMutations& sceneMutations,
                      EditorAssetRepository& assetRepository,
                      MaterialMutations& materialMutations,
                      ObjectBuilder& objectBuilder,
                      SceneLoader& editorWorkspace,
                      GameWindow& window,
                      GameEngine& engine,
                      UndoHistory& undoHistory,
                      ActionDispatcher& actionDispatcher,
                      ShortcutBindingService& shortcutBindingService);

    // --------------------------------------------------------
    // Rendering
    // --------------------------------------------------------
    void render(const EntityManager& entityManager, float gridScale, const std::vector<DrawCall>& outlineQueue);

private:
    void buildGizmos();
    void buildOutlines();

    VulkanEditorBackend& renderer_;
    EditorCamera& editorOrbitCamera_;
    EditorSelection& editorSelection_;
    EditorGizmos& editorGizmos_;
    EditorSettings& editorSettings_;
    RendererSettings& rendererSettings_;
    EntityManager& entityManager_;
    GizmosRenderer& gizmosBuilder_;
    ObjectTransformHandle& objectTransformHandle_;
    UserInterface& userInterface_;
    PickingSystem& pickingSystem_;
    SceneMutations& sceneMutations_;
    EditorAssetRepository& assetRepository_;
    ObjectBuilder& objectBuilder_;
    SceneLoader& editorWorkspace_;
    GameEngine& engine_;
    GameWindow& window_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;
    std::vector<DrawCall> outlineQueue_;
    std::vector<VulkanGizmoPass::GizmoVertex> builtGizmoLines_;
};
