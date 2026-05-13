#pragma once
#include <filesystem>
#include "../../engine/GameEngine.hpp"
#include "../../engine/modules/asset_management/AssetCache.hpp"
#include "../../engine/modules/asset_management/AssetLoader.hpp"
#include "../../engine/modules/core/GameWindow.hpp"
#include "../../engine/modules/core/TimeManager.hpp"
#include "../../engine/modules/input/InputSystem.hpp"
#include "../../engine/modules/physics/PhysicsSystem.hpp"
#include "../../engine/modules/rendering/GameRenderSystem.hpp"
#include "../../engine/modules/rendering/vulkan/VulkanGameRenderer.hpp"
#include "../../engine/modules/rendering/vulkan/core/VulkanCommandManager.hpp"
#include "../../engine/modules/rendering/vulkan/core/VulkanDebugMessenger.hpp"
#include "../../engine/modules/rendering/vulkan/core/VulkanFrameManager.hpp"
#include "../../engine/modules/rendering/vulkan/core/VulkanSwapchainManager.hpp"
#include "../../engine/modules/rendering/vulkan/core/resources/VulkanMaterialCache.hpp"
#include "../../engine/modules/rendering/vulkan/core/resources/VulkanMeshBuffersCache.hpp"
#include "../../engine/modules/rendering/vulkan/core/resources/VulkanPipelineCache.hpp"
#include "../../engine/modules/rendering/vulkan/core/resources/VulkanRenderTargetManager.hpp"
#include "../../engine/modules/rendering/vulkan/core/resources/VulkanResourcesManager.hpp"
#include "../../engine/modules/rendering/vulkan/core/utils/initialization.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanGeometryPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanGridPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanLightingPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanObjectIdPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanOutlinePass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanUIPass.hpp"
#include "../../engine/modules/scene/EntityManager.hpp"
#include "../../engine/modules/ui/UserInterface.hpp"
#include "../../engine/structs/RendererSettings.hpp"
#include "EditorApp.hpp"
#include "business/ClipboardService.hpp"
#include "business/EditorCamera.hpp"
#include "business/EditorGizmos.hpp"
#include "business/EditorSettings.hpp"
#include "business/ObjectSelection.hpp"
#include "business/ObjectTransformHandle.hpp"
#include "business/SceneLoader.hpp"
#include "business/UndoHistory.hpp"
#include "business/asset_editing/EditorAssetRepository.hpp"
#include "business/asset_editing/MaterialMutations.hpp"
#include "business/scene_editing/SceneMutations.hpp"
#include "input/InputHandler.hpp"
#include "input/PickingSystem.hpp"
#include "presentation/PresentationLayer.hpp"
#include "presentation/gizmos/GizmosBuilder.hpp"
#include "presentation/vulkan/VulkanEditorBackend.hpp"

/*
    EditorWiringContainer
    --------------------------------------------------
    Central composition root for the editor application.
*/
class EditorWiringContainer {
public:
    explicit EditorWiringContainer(GameWindow& window, const std::filesystem::path& projectPath = "") :
        window_(window),
        assetExplorer_(),
        loadedAssets_(),
        assetLoader_(assetExplorer_, loadedAssets_),
        rendererSettings_(),
        userInterface_(),
        entityManager_(),
        physicsSystem_(entityManager_),
        undoHistory_(),
        clipboardService_(),
        assetRepository_(assetLoader_, assetExplorer_, loadedAssets_, undoHistory_, projectPath / "assets"),
        vulkanContext_(initVulkanContext()),
        debugMessenger_(vulkanContext_),
        swapchainManager_(window_, vulkanContext_),
        commandManager_(vulkanContext_, swapchainManager_),
        frameManager_(vulkanContext_, commandManager_, swapchainManager_),
        pipelineCache_(vulkanContext_),
        meshBuffersCache_(vulkanContext_),
        textureCache_(vulkanContext_),
        materialCache_(vulkanContext_, pipelineCache_, textureCache_, assetLoader_),
        resourcesManager_(meshBuffersCache_, textureCache_, pipelineCache_, materialCache_),
        renderTargetManager_(vulkanContext_),
        geometryPass_(vulkanContext_, resourcesManager_, assetLoader_, window_),
        lightingPass_(
                vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
        gameRenderer_(vulkanContext_,
                      frameManager_,
                      renderTargetManager_,
                      geometryPass_,
                      lightingPass_,
                      swapchainManager_,
                      rendererSettings_),
        gameRenderSystem_(gameRenderer_),
        inputSystem_(window_),
        time_([&window] { return static_cast<float>(window.getTime()); }),
        engine_(window_,
                time_,
                loadedAssets_,
                inputSystem_,
                physicsSystem_,
                entityManager_,
                gameRenderSystem_,
                rendererSettings_,
                gameRenderer_),
        gridPass_(assetLoader_, resourcesManager_),
        gizmoPass_(vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_),
        objectIdPass_(vulkanContext_, assetLoader_, resourcesManager_),
        outlinePass_(
                vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
        uiPass_(vulkanContext_, swapchainManager_, window, userInterface_),
        vulkanEditorRenderer_(window,
                              vulkanContext_,
                              frameManager_,
                              renderTargetManager_,
                              geometryPass_,
                              lightingPass_,
                              gridPass_,
                              gizmoPass_,
                              objectIdPass_,
                              outlinePass_,
                              uiPass_,
                              swapchainManager_,
                              rendererSettings_),
        projectPath_(projectPath),
        editorSettings_(rendererSettings_),
        editorCamera_(),
        gizmosBuilder_(assetLoader_, entityManager_, editorSettings_),
        pickingSystem_(assetLoader_),
        sceneMutations_(entityManager_, engine_, objectSelection_, undoHistory_, clipboardService_),
        materialMutations_(assetRepository_, objectSelection_),
        sceneLoader_(entityManager_, objectSelection_, engine_, assetRepository_),
        objectTransformHandle_(
                window, engine_, sceneMutations_, editorCamera_, objectSelection_, editorSettings_, entityManager_),
        presentationLayer_(vulkanEditorRenderer_,
                           editorCamera_,
                           objectSelection_,
                           editorGizmos_,
                           editorSettings_,
                           rendererSettings_,
                           entityManager_,
                           gizmosBuilder_,
                           objectTransformHandle_,
                           userInterface_,
                           pickingSystem_,
                           sceneMutations_,
                           assetRepository_,
                           materialMutations_,
                           sceneLoader_,
                           window,
                           engine_,
                           undoHistory_),
        inputHandler_(window,
                      engine_,
                      pickingSystem_,
                      editorCamera_,
                      objectSelection_,
                      objectTransformHandle_,
                      entityManager_,
                      sceneMutations_,
                      editorGizmos_,
                      editorSettings_,
                      undoHistory_,
                      clipboardService_),
        editorApp_(window,
                   engine_,
                   entityManager_,
                   editorCamera_,
                   inputHandler_,
                   presentationLayer_,
                   editorSettings_,
                   sceneLoader_,
                   time_,
                   inputSystem_,
                   physicsSystem_,
                   assetExplorer_,
                   projectPath_) {}

    EditorApp& editorApp() { return editorApp_; }

private:
    // Window (external reference)
    GameWindow& window_;

    AssetExplorer assetExplorer_;
    AssetCache loadedAssets_;
    AssetLoader assetLoader_;

    // Engine core
    RendererSettings rendererSettings_;
    UserInterface userInterface_;
    EntityManager entityManager_;
    PhysicsSystem physicsSystem_;
    UndoHistory undoHistory_;
    EditorAssetRepository assetRepository_;

    // Vulkan core
    VulkanContext vulkanContext_;
    VulkanDebugMessenger debugMessenger_;
    VulkanSwapchainManager swapchainManager_;
    VulkanCommandManager commandManager_;
    VulkanFrameManager frameManager_;

    // Vulkan resource caches
    VulkanPipelineCache pipelineCache_;
    VulkanMeshBuffersCache meshBuffersCache_;
    VulkanTextureCache textureCache_;
    VulkanMaterialCache materialCache_;
    VulkanResourcesManager resourcesManager_;
    VulkanRenderTargetManager renderTargetManager_;

    // Vulkan render passes
    VulkanGeometryPass geometryPass_;
    VulkanLightingPass lightingPass_;
    VulkanGameRenderer gameRenderer_;
    GameRenderSystem gameRenderSystem_;
    InputSystem inputSystem_;
    TimeManager time_;
    GameEngine engine_;

    // Vulkan editor passes
    VulkanGridPass gridPass_;
    VulkanGizmoPass gizmoPass_;
    VulkanObjectIdPass objectIdPass_;
    VulkanOutlinePass outlinePass_;
    VulkanUIPass uiPass_;
    VulkanEditorBackend vulkanEditorRenderer_;

    // Project metadata
    std::filesystem::path projectPath_;

    // Business logic
    ObjectSelection objectSelection_;
    EditorGizmos editorGizmos_;
    EditorSettings editorSettings_;
    EditorCamera editorCamera_;
    GizmosRenderer gizmosBuilder_;
    PickingSystem pickingSystem_;
    ClipboardService clipboardService_;
    SceneMutations sceneMutations_;
    MaterialMutations materialMutations_;
    SceneLoader sceneLoader_;
    ObjectTransformHandle objectTransformHandle_;

    // Presentation and input handling
    PresentationLayer presentationLayer_;
    InputHandler inputHandler_;

    // Root application
    EditorApp editorApp_;
};
