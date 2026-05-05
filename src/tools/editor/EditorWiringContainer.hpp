#pragma once
#include <filesystem>
#include "../../engine/GameEngineWiringContainer.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanGridPass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanObjectIdPass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanOutlinePass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanUIPass.hpp"
#include "EditorApp.hpp"
#include "business/EditorGizmos.hpp"
#include "business/EditorOrbitCamera.hpp"
#include "business/EditorSettings.hpp"
#include "business/ObjectSelection.hpp"
#include "business/ObjectTransformHandle.hpp"
#include "business/SceneLoader.hpp"
#include "input/InputHandler.hpp"
#include "input/PickingSystem.hpp"
#include "presentation/PresentationLayer.hpp"
#include "presentation/gizmos/GizmosRenderer.hpp"
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
        editorOrbitCamera_(),
        gizmosBuilder_(assetManager_, scene_),
        pickingSystem_(assetManager_),
        sceneLoader_(scene_, objectSelection_, engine_),
        sceneMutations_(scene_, engine_, &objectSelection_),
        objectTransformHandle_(
                window, engine_, sceneMutations_, editorOrbitCamera_, objectSelection_, editorSettings_, scene_),
        // -------------------------------------------------------------------------------------------------------------
        // Presentation and input handling
        // -------------------------------------------------------------------------------------------------------------
        presentationLayer_(vulkanEditorRenderer_,
                           editorOrbitCamera_,
                           objectSelection_,
                           editorGizmos_,
                           editorSettings_,
                           rendererSettings_,
                           scene_,
                           gizmosBuilder_,
                           objectTransformHandle_,
                           userInterface_,
                           pickingSystem_,
                           sceneMutations_,
                           sceneLoader_,
                           assetManager_),
        inputHandler_(window,
                      engine_,
                      pickingSystem_,
                      editorOrbitCamera_,
                      objectSelection_,
                      objectTransformHandle_,
                      scene_,
                      sceneMutations_,
                      editorGizmos_,
                      editorSettings_),
        // -------------------------------------------------------------------------------------------------------------
        // Editor application entry point (root)
        // -------------------------------------------------------------------------------------------------------------
        editorApp_(window,
                   engine_,
                   scene_,
                   editorOrbitCamera_,
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
    EditorOrbitCamera editorOrbitCamera_;
    GizmosRenderer gizmosBuilder_;
    PickingSystem pickingSystem_;
    SceneLoader sceneLoader_;
    SceneMutations sceneMutations_;
    ObjectTransformHandle objectTransformHandle_;

    // Presentation and input handling
    PresentationLayer presentationLayer_;
    InputHandler inputHandler_;

    // Root application
    EditorApp editorApp_;
};
