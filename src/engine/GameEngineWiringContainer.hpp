#pragma once
#include <filesystem>
#include "GameEngine.hpp"
#include "modules/assets/AssetImporter.hpp"
#include "modules/assets/AssetManager.hpp"
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
#include "modules/rendering/vulkan/core/resources/VulkanMaterialCache.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanMeshBuffersCache.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanPipelineCache.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanRenderTargetManager.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanResourcesManager.hpp"
#include "modules/rendering/vulkan/core/utils/initialization.hpp"
#include "modules/rendering/vulkan/passes/VulkanGeometryPass.hpp"
#include "modules/rendering/vulkan/passes/VulkanLightingPass.hpp"
#include "modules/scene/EntityManager.hpp"
#include "modules/ui/UserInterface.hpp"
#include "structs/settings/RendererSettings.hpp"

/*
    GameEngineWiringContainer

    Purpose:
    --------------------------------------------------
    Central composition root for the game engine.

    Responsibilities:
    --------------------------------------------------
    - Create and wire dependencies for core game engine business/
    - Own and manage the complete object graph for the game
    - Manage Vulkan rendering subsystem
    - Expose game engine systems and rendering APIs

    This class is not responsible for:
    --------------------------------------------------
    - Editor-specific functionality
    - Application logic
    - Public API for the application layer
*/
class GameEngineWiringContainer {
public:
    GameEngineWiringContainer(GameWindow& window, const std::filesystem::path& assetsPath) :
        window_(window),
        rendererSettings_(),
        userInterface_(),
        assetStorage_(),
        assetImporter_(assetsPath, assetStorage_),
        assetManager_(assetImporter_, assetStorage_),
        entityManager_(),
        physicsSystem_(entityManager_),
        vulkanContext_(initVulkanContext()),
        debugMessenger_(vulkanContext_),
        swapchainManager_(window_, vulkanContext_),
        commandManager_(vulkanContext_, swapchainManager_),
        frameManager_(vulkanContext_, commandManager_, swapchainManager_),
        pipelineCache_(vulkanContext_),
        meshBuffersCache_(vulkanContext_),
        textureCache_(vulkanContext_),
        materialCache_(vulkanContext_, pipelineCache_, textureCache_, assetManager_),
        resourcesManager_(meshBuffersCache_, textureCache_, pipelineCache_, materialCache_),
        renderTargetManager_(vulkanContext_),
        geometryPass_(vulkanContext_, resourcesManager_, assetManager_, window_),
        lightingPass_(
                vulkanContext_, assetManager_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
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
                assetManager_,
                inputSystem_,
                physicsSystem_,
                entityManager_,
                gameRenderSystem_,
                rendererSettings_,
                gameRenderer_) {}

    GameEngine& gameEngine() { return engine_; }
    UserInterface& userInterface() { return userInterface_; }
    RendererSettings& rendererSettings() { return rendererSettings_; }
    EntityManager& entityManager() { return entityManager_; }
    AssetManager& assetManager() { return assetManager_; }
    TimeManager& timeManager() { return time_; }
    InputSystem& inputSystem() { return inputSystem_; }
    PhysicsSystem& physicsSystem() { return physicsSystem_; }

protected:
    GameWindow& window_;
    RendererSettings rendererSettings_;
    UserInterface userInterface_;
    AssetStorage assetStorage_;
    AssetImporter assetImporter_;
    AssetManager assetManager_;
    EntityManager entityManager_;
    PhysicsSystem physicsSystem_;

    // Vulkan components
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
};
