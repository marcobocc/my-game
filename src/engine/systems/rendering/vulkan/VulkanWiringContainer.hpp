#pragma once
#include "VulkanCommandManager.hpp"
#include "VulkanDebugMessenger.hpp"
#include "VulkanGraphicsBackend.hpp"
#include "VulkanPickingBackend.hpp"
#include "VulkanRenderingOrchestrator.hpp"
#include "passes/VulkanGeometryPass.hpp"
#include "passes/VulkanGizmoPass.hpp"
#include "passes/VulkanGridPass.hpp"
#include "passes/VulkanLightingPass.hpp"
#include "passes/VulkanObjectIdPass.hpp"
#include "passes/VulkanOutlinePass.hpp"
#include "passes/VulkanUIPass.hpp"
#include "resources/VulkanMaterialCache.hpp"
#include "resources/VulkanMeshBuffersCache.hpp"
#include "resources/VulkanPipelineCache.hpp"
#include "resources/VulkanResourcesManager.hpp"
#include "resources/VulkanTextureCache.hpp"
#include "systems/assets/AssetManager.hpp"
#include "utils/initialization.hpp"
#include "utils/structs.hpp"

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
        geometryPass_(vulkanContext_, resourcesManager_, assetManager, window),
        lightingPass_(vulkanContext_,
                      assetManager,
                      resourcesManager_,
                      swapchainManager_.swapchain().swapchainImageFormat,
                      static_cast<uint32_t>(swapchainManager_.swapchain().swapchainImages.size())),
        gridPass_(assetManager, resourcesManager_, swapchainManager_),
        gizmoPass_(vulkanContext_, assetManager, resourcesManager_, swapchainManager_),
        objectIdPass_(vulkanContext_, assetManager, resourcesManager_),
        pickingBackend_(vulkanContext_),
        outlinePass_(vulkanContext_,
                     assetManager,
                     resourcesManager_,
                     swapchainManager_.swapchain().swapchainImageFormat,
                     static_cast<uint32_t>(swapchainManager_.swapchain().swapchainImages.size()),
                     swapchainManager_.swapchain().swapchainExtent.width,
                     swapchainManager_.swapchain().swapchainExtent.height),
        uiPass_(vulkanContext_, swapchainManager_, window, userInterface),
        renderingOrchestrator_(window,
                               vulkanContext_,
                               geometryPass_,
                               lightingPass_,
                               gridPass_,
                               gizmoPass_,
                               objectIdPass_,
                               pickingBackend_,
                               outlinePass_,
                               uiPass_,
                               commandManager_,
                               swapchainManager_,
                               settings),
        graphicsBackend_(vulkanContext_, renderingOrchestrator_, pickingBackend_) {}

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

    VulkanGeometryPass geometryPass_;
    VulkanLightingPass lightingPass_;
    VulkanGridPass gridPass_;
    VulkanGizmoPass gizmoPass_;
    VulkanObjectIdPass objectIdPass_;
    VulkanPickingBackend pickingBackend_;
    VulkanOutlinePass outlinePass_;
    VulkanUIPass uiPass_;
    VulkanRenderingOrchestrator renderingOrchestrator_;

    VulkanGraphicsBackend graphicsBackend_;
};
