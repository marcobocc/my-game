#pragma once
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "../core/structs.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/RendererSettings.hpp"

// ----------------------------------------------------------------------------
// Forward definitions
// ----------------------------------------------------------------------------
class GameWindow;
struct VulkanContext;
class VulkanCommandManager;
class VulkanSceneRenderer;
class VulkanGridRenderer;
class VulkanPickingRenderer;
class VulkanImguiRenderer;
class VulkanSwapchainManager;

// ----------------------------------------------------------------------------
// VulkanRenderingOrchestrator
// ----------------------------------------------------------------------------
class VulkanRenderingOrchestrator {
public:
    VulkanRenderingOrchestrator(GameWindow& window,
                                VulkanContext& context,
                                VulkanSceneRenderer& sceneRenderer,
                                VulkanGridRenderer& gridRenderer,
                                VulkanPickingRenderer& pickingRenderer,
                                VulkanImguiRenderer& imguiRenderer,
                                VulkanCommandManager& commandManager,
                                VulkanSwapchainManager& swapchainManager,
                                RendererSettings& settings);

    ~VulkanRenderingOrchestrator();

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId) const;
    bool renderFrame(const Camera& camera, const Transform& cameraTransform);

    void requestPick(uint32_t x, uint32_t y);
    std::optional<std::string> getPickResult();

private:
    // Frame stages
    bool acquireImage(uint32_t& imageIndex);
    VkCommandBuffer beginFrame() const;
    void prepareSceneCanvas(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void recordCommands(VkCommandBuffer, uint32_t imageIndex, const Camera& camera, const Transform& cameraTransform);
    void endFrame(VkCommandBuffer, uint32_t imageIndex);
    void submit(VkCommandBuffer, uint32_t imageIndex);

    // Frame synchronization
    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
    };
    std::vector<FrameSync> frames_;
    size_t currentFrameIndex_ = 0;
    void initFrameSync();
    void destroyFrameSync();
    void waitForCurrentFrame();
    void advanceFrame();
    FrameSync& currentFrame();

    // Layout transitions
    void transitionToRenderLayouts(VkCommandBuffer, uint32_t imageIndex) const;
    void transitionToPresent(VkCommandBuffer, uint32_t imageIndex) const;
    static void transitionColorAttachment(VkCommandBuffer cmd, VkImage image);
    static void transitionDepthAttachment(VkCommandBuffer cmd, VkImage image);
    static void transitionToPresent(VkCommandBuffer cmd, VkImage image);

    GameWindow& window_;
    VulkanContext& context_;
    VulkanSceneRenderer& sceneRenderer_;
    VulkanGridRenderer& gridRenderer_;
    VulkanPickingRenderer& pickingRenderer_;
    VulkanImguiRenderer& imguiRenderer_;
    VulkanCommandManager& commandManager_;
    VulkanSwapchainManager& swapchainManager_;
    RendererSettings& settings_;

    bool hasPendingPickRequest_ = false;
    uint32_t pendingPickX_ = 0;
    uint32_t pendingPickY_ = 0;
    bool pickResultReady_ = false;
    std::optional<std::string> pendingPickResult_;
};
