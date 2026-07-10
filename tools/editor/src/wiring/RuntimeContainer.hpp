#pragma once
#include <filesystem>
#include "../../../../runtime/src/graphics/RendererSettings.hpp"
#include "animation/AnimationSystem.hpp"
#include "console/DeveloperConsole.hpp"
#include "core/GameWindow.hpp"
#include "core/TimeManager.hpp"
#include "core/assets/AssetCache.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/VirtualFileSystem.hpp"
#include "core/scene/World.hpp"
#include "graphics/GameRenderSystem.hpp"
#include "graphics/vulkan/core/VulkanCommandManager.hpp"
#include "graphics/vulkan/core/VulkanDebugMessenger.hpp"
#include "graphics/vulkan/core/VulkanFrameManager.hpp"
#include "graphics/vulkan/core/VulkanSwapchainManager.hpp"
#include "graphics/vulkan/core/resources/VulkanFontCache.hpp"
#include "graphics/vulkan/core/resources/VulkanMaterialCache.hpp"
#include "graphics/vulkan/core/resources/VulkanMeshBuffersCache.hpp"
#include "graphics/vulkan/core/resources/VulkanPipelineCache.hpp"
#include "graphics/vulkan/core/resources/VulkanRenderTargetManager.hpp"
#include "graphics/vulkan/core/resources/VulkanResourcesManager.hpp"
#include "graphics/vulkan/core/utils/initialization.hpp"
#include "graphics/vulkan/passes/VulkanGeometryPass.hpp"
#include "graphics/vulkan/passes/VulkanGizmoPass.hpp"
#include "graphics/vulkan/passes/VulkanGridPass.hpp"
#include "graphics/vulkan/passes/VulkanLightingPass.hpp"
#include "graphics/vulkan/passes/VulkanObjectIdPass.hpp"
#include "graphics/vulkan/passes/VulkanOutlinePass.hpp"
#include "graphics/vulkan/passes/VulkanUIPass.hpp"
#include "input/InputSystem.hpp"
#include "physics/PhysicsSystem.hpp"
#include "rendering/VulkanBackend.hpp"
#include "services/AssetThumbnailGenerator.hpp"
#include "services/SimulationController.hpp"

class RuntimeContainer {
public:
    explicit RuntimeContainer(GameWindow& window,
                              const std::vector<std::filesystem::path>& mountPaths,
                              const std::filesystem::path& projectRoot) :
        projectRoot_(projectRoot),
        vfs_(mountPaths, projectRoot),
        assetLoader_(vfs_, loadedAssets_),
        physicsSystem_(world_),
        vulkanContext_(initVulkanContext()),
        debugMessenger_(vulkanContext_),
        swapchainManager_(window, vulkanContext_),
        commandManager_(vulkanContext_, swapchainManager_),
        frameManager_(vulkanContext_, commandManager_, swapchainManager_),
        pipelineCache_(vulkanContext_),
        meshBuffersCache_(vulkanContext_),
        textureCache_(vulkanContext_),
        materialCache_(vulkanContext_, pipelineCache_, textureCache_, assetLoader_),
        fontCache_(vulkanContext_),
        assetThumbnailGenerator_(assetLoader_, textureCache_, projectRoot),
        resourcesManager_(meshBuffersCache_, textureCache_, pipelineCache_, materialCache_, fontCache_),
        renderTargetManager_(vulkanContext_),
        geometryPass_(vulkanContext_, resourcesManager_, assetLoader_, window),
        lightingPass_(
                vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
        inputSystem_(window),
        time_([&window] { return static_cast<float>(window.getTime()); }),
        gridPass_(assetLoader_, resourcesManager_),
        gizmoPass_(vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_),
        gizmoOverlayPass_(vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_, GIZMO_OVERLAY_SHADER),
        objectIdPass_(vulkanContext_, assetLoader_, resourcesManager_),
        outlinePass_(
                vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
        uiPass_(vulkanContext_, swapchainManager_, window),
        vulkanBackend_(window,
                       vulkanContext_,
                       frameManager_,
                       renderTargetManager_,
                       geometryPass_,
                       lightingPass_,
                       gridPass_,
                       gizmoPass_,
                       gizmoOverlayPass_,
                       objectIdPass_,
                       outlinePass_,
                       uiPass_,
                       swapchainManager_,
                       rendererSettings_,
                       resourcesManager_,
                       assetLoader_),
        gameRenderSystem_(vulkanBackend_),
        simulationController_(window,
                              time_,
                              loadedAssets_,
                              assetLoader_,
                              inputSystem_,
                              gameRenderSystem_,
                              rendererSettings_,
                              vulkanBackend_) {}

    DeveloperConsole& developerConsole() { return developerConsole_; }
    World& entityManager() { return world_; }
    InputSystem& inputSystem() { return inputSystem_; }
    PhysicsSystem& physicsSystem() { return physicsSystem_; }
    TimeManager& time() { return time_; }
    AssetThumbnailGenerator& assetThumbnailGenerator() { return assetThumbnailGenerator_; }
    VirtualFileSystem& vfs() { return vfs_; }
    AssetLoader& assetLoader() { return assetLoader_; }
    AssetCache& loadedAssets() { return loadedAssets_; }
    RendererSettings& rendererSettings() { return rendererSettings_; }
    VulkanContext& vulkanContext() { return vulkanContext_; }
    VulkanFrameManager& frameManager() { return frameManager_; }
    VulkanRenderTargetManager& renderTargetManager() { return renderTargetManager_; }
    VulkanResourcesManager& resourcesManager() { return resourcesManager_; }
    VulkanGeometryPass& geometryPass() { return geometryPass_; }
    VulkanLightingPass& lightingPass() { return lightingPass_; }
    VulkanGridPass& gridPass() { return gridPass_; }
    VulkanGizmoPass& gizmoPass() { return gizmoPass_; }
    VulkanGizmoPass& gizmoOverlayPass() { return gizmoOverlayPass_; }
    VulkanObjectIdPass& objectIdPass() { return objectIdPass_; }
    VulkanOutlinePass& outlinePass() { return outlinePass_; }
    VulkanUIPass& uiPass() { return uiPass_; }
    VulkanSwapchainManager& swapchainManager() { return swapchainManager_; }
    VulkanBackend& vulkanBackend() { return vulkanBackend_; }
    SimulationController& simulationController() { return simulationController_; }
    AnimationSystem& animationSystem() { return animationSystem_; }
    const std::filesystem::path& projectRoot() const { return projectRoot_; }

private:
    std::filesystem::path projectRoot_;
    VirtualFileSystem vfs_;
    AssetLoader assetLoader_;
    AssetCache loadedAssets_;
    RendererSettings rendererSettings_;
    World world_;
    PhysicsSystem physicsSystem_;
    VulkanContext vulkanContext_;
    VulkanDebugMessenger debugMessenger_;
    VulkanSwapchainManager swapchainManager_;
    VulkanCommandManager commandManager_;
    VulkanFrameManager frameManager_;
    VulkanPipelineCache pipelineCache_;
    VulkanMeshBuffersCache meshBuffersCache_;
    VulkanTextureCache textureCache_;
    VulkanMaterialCache materialCache_;
    VulkanFontCache fontCache_;
    AssetThumbnailGenerator assetThumbnailGenerator_;
    VulkanResourcesManager resourcesManager_;
    VulkanRenderTargetManager renderTargetManager_;
    VulkanGeometryPass geometryPass_;
    VulkanLightingPass lightingPass_;
    InputSystem inputSystem_;
    TimeManager time_;
    DeveloperConsole developerConsole_;
    VulkanGridPass gridPass_;
    VulkanGizmoPass gizmoPass_;
    VulkanGizmoPass gizmoOverlayPass_;
    VulkanObjectIdPass objectIdPass_;
    VulkanOutlinePass outlinePass_;
    VulkanUIPass uiPass_;
    VulkanBackend vulkanBackend_;
    GameRenderSystem gameRenderSystem_;
    SimulationController simulationController_;
    AnimationSystem animationSystem_{assetLoader_};
};
