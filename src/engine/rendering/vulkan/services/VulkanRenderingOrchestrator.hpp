#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "../core/structs.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Mesh.hpp"
#include "core/objects/components/Transform.hpp"

// ----------------------------------------------------------------------------
// Forward definitions
// ----------------------------------------------------------------------------
class GameWindow;
struct VulkanContext;
class VulkanCommandManager;
class VulkanSceneRenderer;
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
                                VulkanImguiRenderer& imguiRenderer,
                                VulkanCommandManager& commandManager,
                                VulkanSwapchainManager& swapchainManager);

    ~VulkanRenderingOrchestrator();

    void enqueueForDrawing(const Mesh&, const Material&, const Transform&) const;
    bool renderFrame(const Camera& camera);

private:
    // Frame stages
    bool acquireImage(uint32_t& imageIndex);
    VkCommandBuffer beginFrame() const;
    void prepareSceneCanvas(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void recordCommands(VkCommandBuffer, uint32_t imageIndex, const Camera& camera);
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
    VulkanImguiRenderer& imguiRenderer_;
    VulkanCommandManager& commandManager_;
    VulkanSwapchainManager& swapchainManager_;
};
