#pragma once
#include <filesystem>
#include "../../engine/GameEngineWiringContainer.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanGridPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanObjectIdPass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanOutlinePass.hpp"
#include "../../engine/modules/rendering/vulkan/passes/VulkanUIPass.hpp"
#include "EditorApp.hpp"
#include "business/EditorCamera.hpp"
#include "business/EditorGizmos.hpp"
#include "business/EditorSettings.hpp"
#include "business/ObjectSelection.hpp"
#include "business/ObjectTransformHandle.hpp"
#include "business/SceneLoader.hpp"
#include "business/asset_editing/MaterialMutations.hpp"
#include "business/scene_editing/SceneMutations.hpp"
#include "business/scene_editing/UndoHistory.hpp"
#include "input/InputHandler.hpp"
#include "input/PickingSystem.hpp"
#include "presentation/PresentationLayer.hpp"
#include "presentation/gizmos/GizmosBuilder.hpp"
#include "presentation/vulkan/VulkanEditorBackend.hpp"

/*
    EditorWiringContainer
    --------------------------------------------------
    Central composition root for the editor application
*/
class EditorWiringContainer : public GameEngineWiringContainer {
public:
    EditorWiringContainer(GameWindow& window, const std::filesystem::path& assetsPath) :
        GameEngineWiringContainer(window, assetsPath),
        // -------------------------------------------------------------------------------------------------------------
        // Vulkan backend for the editor
        // -------------------------------------------------------------------------------------------------------------
        gridPass_(assetManager_, resourcesManager_),
        gizmoPass_(vulkanContext_, assetManager_, resourcesManager_, swapchainManager_),
        objectIdPass_(vulkanContext_, assetManager_, resourcesManager_),
        outlinePass_(
                vulkanContext_, assetManager_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
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
        // -------------------------------------------------------------------------------------------------------------
        // Editor business logic providers
        // -------------------------------------------------------------------------------------------------------------
        editorSettings_(rendererSettings_),
        editorCamera_(),
        gizmosBuilder_(assetManager_, entityManager_, editorSettings_),
        pickingSystem_(assetManager_),
        sceneLoader_(entityManager_, objectSelection_, engine_),
        undoHistory_(),
        sceneMutations_(entityManager_, engine_, objectSelection_, undoHistory_),
        materialMutations_(assetManager_, objectSelection_, undoHistory_),
        objectTransformHandle_(
                window, engine_, sceneMutations_, editorCamera_, objectSelection_, editorSettings_, entityManager_),
        // -------------------------------------------------------------------------------------------------------------
        // Presentation and input handling
        // -------------------------------------------------------------------------------------------------------------
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
                           materialMutations_,
                           sceneLoader_,
                           assetManager_,
                           window,
                           engine_),
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
                      undoHistory_),
        // -------------------------------------------------------------------------------------------------------------
        // Editor application entry point (root)
        // -------------------------------------------------------------------------------------------------------------
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
                   physicsSystem_) {}

    EditorApp& editorApp() { return editorApp_; }

private:
    // Vulkan backend
    VulkanGridPass gridPass_;
    VulkanGizmoPass gizmoPass_;
    VulkanObjectIdPass objectIdPass_;
    VulkanOutlinePass outlinePass_;
    VulkanUIPass uiPass_;
    VulkanEditorBackend vulkanEditorRenderer_;

    // Business logic
    ObjectSelection objectSelection_;
    EditorGizmos editorGizmos_;
    EditorSettings editorSettings_;
    EditorCamera editorCamera_;
    GizmosRenderer gizmosBuilder_;
    PickingSystem pickingSystem_;
    SceneLoader sceneLoader_;
    UndoHistory undoHistory_;
    SceneMutations sceneMutations_;
    MaterialMutations materialMutations_;
    ObjectTransformHandle objectTransformHandle_;

    // Presentation and input handling
    PresentationLayer presentationLayer_;
    InputHandler inputHandler_;

    // Root application
    EditorApp editorApp_;
};
