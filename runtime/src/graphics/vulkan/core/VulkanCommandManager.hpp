#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanSwapchainManager.hpp"
#include "utils/structs.hpp"

class VulkanCommandManager {
public:
    VulkanCommandManager(const VulkanCommandManager&) = delete;
    VulkanCommandManager& operator=(const VulkanCommandManager&) = delete;
    VulkanCommandManager(VulkanCommandManager&&) = delete;
    VulkanCommandManager& operator=(VulkanCommandManager&&) = delete;

    ~VulkanCommandManager();
    explicit VulkanCommandManager(const VulkanContext& vulkanContext, const VulkanSwapchainManager& swapchainManager);

    VkCommandBuffer beginFrame();
    void endFrame(VkCommandBuffer cmd);
    void advanceFrame();

    VkCommandBuffer currentCommandBuffer() const;
    void submitCommandBuffer(VkCommandBuffer cmd,
                             VkSemaphore waitSemaphore = VK_NULL_HANDLE,
                             VkSemaphore signalSemaphore = VK_NULL_HANDLE,
                             VkFence fence = VK_NULL_HANDLE);

private:
    void createCommandPools();
    void createFences();

    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    static void beginCommandBuffer(VkCommandBuffer cmd,
                                   VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    static void endCommandBuffer(VkCommandBuffer cmd);

    struct FrameContext {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkFence frameFence = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    };

    VkDevice device_;
    uint32_t graphicsQueueFamilyIndex_;
    VkQueue graphicsQueue_;
    std::vector<FrameContext> frames_;
    size_t maxFramesInFlight_;
    size_t currentFrame_{0};
};
