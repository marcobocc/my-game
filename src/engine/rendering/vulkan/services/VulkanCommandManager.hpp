#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "../core/structs.hpp"
#include "VulkanSwapchainManager.hpp"

class VulkanCommandManager {
public:
    VulkanCommandManager(const VulkanCommandManager&) = delete;
    VulkanCommandManager& operator=(const VulkanCommandManager&) = delete;
    VulkanCommandManager(VulkanCommandManager&&) = delete;
    VulkanCommandManager& operator=(VulkanCommandManager&&) = delete;

    ~VulkanCommandManager();
    explicit VulkanCommandManager(const VulkanContext& vulkanContext, const VulkanSwapchainManager& swapchainManager);
    void beginFrame();
    void endFrame();
    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void submitCommandBuffer(VkCommandBuffer cmd,
                             VkSemaphore waitSemaphore = VK_NULL_HANDLE,
                             VkSemaphore signalSemaphore = VK_NULL_HANDLE,
                             VkFence fence = VK_NULL_HANDLE);
    static void beginCommandBuffer(VkCommandBuffer cmd,
                                   VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    static void endCommandBuffer(VkCommandBuffer cmd);

private:
    void createCommandPools();
    void createFences();

    struct FrameContext {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkFence frameFence = VK_NULL_HANDLE;
    };

    VkDevice device_;
    uint32_t graphicsQueueFamilyIndex_;
    VkQueue graphicsQueue_;
    std::vector<FrameContext> frames_;
    size_t maxFramesInFlight_;
    size_t currentFrame_{0};
};
