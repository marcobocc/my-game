#pragma once

#include <array>
#include <vector>
#include <vulkan/vulkan.h>
#include "vulkan/FrameRenderer.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

class VulkanFramesManager {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanFramesManager(VkDevice device,
                        size_t swapchainImageCount,
                        VulkanPipelinesManager& pipelinesManager,
                        VulkanVertexBuffersManager& vertexBuffersManager,
                        VulkanSwapchainManager& swapchainManager);
    ~VulkanFramesManager();

    VulkanFramesManager(const VulkanFramesManager&) = delete;
    VulkanFramesManager& operator=(const VulkanFramesManager&) = delete;
    VulkanFramesManager(VulkanFramesManager&&) = delete;
    VulkanFramesManager& operator=(VulkanFramesManager&&) = delete;

    bool renderFrame(size_t& currentFrame,
                     VulkanCommandManager& commandManager,
                     const VulkanSwapchainManager& swapchainManager,
                     VkQueue graphicsQueue,
                     const std::vector<DrawCall>& drawCalls) const;

private:
    void createSynchronizationObjects();
    void waitForFrame(size_t frameIndex) const;
    bool acquireImage(size_t currentFrame, const VulkanSwapchainManager& swapchainManager, uint32_t& imageIndex) const;
    void submitAndPresent(VkCommandBuffer cmd,
                          size_t currentFrame,
                          uint32_t imageIndex,
                          VulkanCommandManager& commandManager,
                          const VulkanSwapchainManager& swapchainManager,
                          VkQueue graphicsQueue) const;

    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
    };

    struct ImageSync {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    };

    VkDevice device_;
    std::array<FrameSync, MAX_FRAMES_IN_FLIGHT> frames_{};
    std::vector<ImageSync> images_;
    FrameRenderer frameRenderer_;
};
