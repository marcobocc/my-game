#pragma once
#include <filesystem>
#include "GameEngine.hpp"
#include "modules/asset_management/AssetCache.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/VirtualFileSystem.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/core/TimeManager.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/physics/PhysicsSystem.hpp"
#include "modules/rendering/GameRenderSystem.hpp"
#include "modules/rendering/vulkan/VulkanGameRenderer.hpp"
#include "modules/rendering/vulkan/core/VulkanCommandManager.hpp"
#include "modules/rendering/vulkan/core/VulkanDebugMessenger.hpp"
#include "modules/rendering/vulkan/core/VulkanFrameManager.hpp"
#include "modules/rendering/vulkan/core/VulkanSwapchainManager.hpp"
#include "modules/rendering/vulkan/core/resources/AssetThumbnailGenerator.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanMaterialCache.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanMeshBuffersCache.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanPipelineCache.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanRenderTargetManager.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanResourcesManager.hpp"
#include "modules/rendering/vulkan/core/utils/initialization.hpp"
#include "modules/rendering/vulkan/passes/VulkanGeometryPass.hpp"
#include "modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "modules/rendering/vulkan/passes/VulkanGridPass.hpp"
#include "modules/rendering/vulkan/passes/VulkanLightingPass.hpp"
#include "modules/rendering/vulkan/passes/VulkanObjectIdPass.hpp"
#include "modules/rendering/vulkan/passes/VulkanOutlinePass.hpp"
#include "modules/rendering/vulkan/passes/VulkanUIPass.hpp"
#include "modules/scene/World.hpp"
#include "structs/RendererSettings.hpp"

class RuntimeContainer {
public:
    explicit RuntimeContainer(GameWindow& window,
                              const std::vector<std::filesystem::path>& mountPaths,
                              const std::filesystem::path& projectRoot) :
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
        assetThumbnailGenerator_(assetLoader_, textureCache_),
        resourcesManager_(meshBuffersCache_, textureCache_, pipelineCache_, materialCache_),
        renderTargetManager_(vulkanContext_),
        geometryPass_(vulkanContext_, resourcesManager_, assetLoader_, window),
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
        inputSystem_(window),
        time_([&window] { return static_cast<float>(window.getTime()); }),
        engine_(window,
                time_,
                loadedAssets_,
                inputSystem_,
                physicsSystem_,
                world_,
                gameRenderSystem_,
                rendererSettings_,
                gameRenderer_),
        gridPass_(assetLoader_, resourcesManager_),
        gizmoPass_(vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_),
        objectIdPass_(vulkanContext_, assetLoader_, resourcesManager_),
        outlinePass_(
                vulkanContext_, assetLoader_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
        uiPass_(vulkanContext_, swapchainManager_, window) {}

    GameEngine& engine() { return engine_; }
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
    VulkanGeometryPass& geometryPass() { return geometryPass_; }
    VulkanLightingPass& lightingPass() { return lightingPass_; }
    VulkanGridPass& gridPass() { return gridPass_; }
    VulkanGizmoPass& gizmoPass() { return gizmoPass_; }
    VulkanObjectIdPass& objectIdPass() { return objectIdPass_; }
    VulkanOutlinePass& outlinePass() { return outlinePass_; }
    VulkanUIPass& uiPass() { return uiPass_; }
    VulkanSwapchainManager& swapchainManager() { return swapchainManager_; }

private:
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
    AssetThumbnailGenerator assetThumbnailGenerator_;
    VulkanResourcesManager resourcesManager_;
    VulkanRenderTargetManager renderTargetManager_;
    VulkanGeometryPass geometryPass_;
    VulkanLightingPass lightingPass_;
    VulkanGameRenderer gameRenderer_;
    GameRenderSystem gameRenderSystem_;
    InputSystem inputSystem_;
    TimeManager time_;
    GameEngine engine_;
    VulkanGridPass gridPass_;
    VulkanGizmoPass gizmoPass_;
    VulkanObjectIdPass objectIdPass_;
    VulkanOutlinePass outlinePass_;
    VulkanUIPass uiPass_;
};
