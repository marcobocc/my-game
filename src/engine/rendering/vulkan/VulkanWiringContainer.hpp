#pragma once
#include <GLFW/glfw3.h>
#include "assets/AssetManager.hpp"
#include "core/initialization.hpp"
#include "core/structs.hpp"
#include "rendering/RendererSettings.hpp"
#include "rendering/vulkan/services/VulkanCommandManager.hpp"
#include "resources/VulkanMaterialCache.hpp"
#include "resources/VulkanMeshBuffersCache.hpp"
#include "resources/VulkanPipelineCache.hpp"
#include "resources/VulkanResourcesManager.hpp"
#include "resources/VulkanTextureCache.hpp"
#include "services/VulkanGraphicsBackend.hpp"
#include "services/VulkanRenderingOrchestrator.hpp"
#include "services/debug/VulkanDebugMessenger.hpp"
#include "services/passes/VulkanGridPass.hpp"
#include "services/passes/VulkanPickingPass.hpp"
#include "services/passes/VulkanScenePass.hpp"
#include "services/passes/VulkanUIPass.hpp"

/*
    VulkanWiringContainer

    Purpose:
    --------------------------------------------------
    Central composition root for the Vulkan subsystem.

    Responsibilities:
    --------------------------------------------------
    - Create and wire dependencies all Vulkan services
    - Expose only the facade class (VulkanGraphicsBackend)

    This class is not responsible for:
    --------------------------------------------------
    - Rendering logic
    - Frame submission
    - UI or application behavior

    Notes:
    --------------------------------------------------
    This class exists purely to build and hold the object graph.
    It should not contain runtime or domain logic, nor it should
    forward logic to other classes. If rendering or application
    behavior appears here, the design is leaking responsibilities.
*/
class VulkanWiringContainer {
public:
    VulkanWiringContainer(GameWindow& window,
                          AssetManager& assetManager,
                          UserInterface& userInterface,
                          RendererSettings& settings) :
        vulkanContext_(initVulkanContext()),
        debugMessenger_(vulkanContext_),
        swapchainManager_(window, vulkanContext_),
        commandManager_(vulkanContext_, swapchainManager_),
        pipelineCache_(vulkanContext_),
        meshBuffersCache_(vulkanContext_),
        textureCache_(vulkanContext_),
        materialCache_(vulkanContext_, pipelineCache_, textureCache_, assetManager),
        resourcesManager_(meshBuffersCache_, textureCache_, pipelineCache_, materialCache_),
        scenePass_(vulkanContext_, resourcesManager_, assetManager, window, swapchainManager_),
        gridPass_(vulkanContext_, assetManager, swapchainManager_),
        pickingPass_(vulkanContext_, assetManager, resourcesManager_, swapchainManager_),
        uiPass_(vulkanContext_, swapchainManager_, window, userInterface),
        renderingOrchestrator_(window,
                               vulkanContext_,
                               scenePass_,
                               gridPass_,
                               pickingPass_,
                               uiPass_,
                               commandManager_,
                               swapchainManager_,
                               settings),
        graphicsBackend_(vulkanContext_, renderingOrchestrator_) {}

    VulkanGraphicsBackend& graphicsBackend() { return graphicsBackend_; }

private:
    VulkanContext vulkanContext_;

    VulkanDebugMessenger debugMessenger_;
    VulkanSwapchainManager swapchainManager_;
    VulkanCommandManager commandManager_;

    VulkanPipelineCache pipelineCache_;
    VulkanMeshBuffersCache meshBuffersCache_;
    VulkanTextureCache textureCache_;
    VulkanMaterialCache materialCache_;
    VulkanResourcesManager resourcesManager_;

    VulkanScenePass scenePass_;
    VulkanGridPass gridPass_;
    VulkanPickingPass pickingPass_;
    VulkanUIPass uiPass_;
    VulkanRenderingOrchestrator renderingOrchestrator_;

    VulkanGraphicsBackend graphicsBackend_;
};
