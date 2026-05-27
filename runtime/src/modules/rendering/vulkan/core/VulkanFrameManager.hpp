#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanContext;
class VulkanCommandManager;
class VulkanSwapchainManager;

class VulkanFrameManager {
public:
    VulkanFrameManager(VulkanContext& context,
                       VulkanCommandManager& commandManager,
                       VulkanSwapchainManager& swapchainManager);

    ~VulkanFrameManager();

    bool acquireImage(uint32_t& imageIndex);
    VkCommandBuffer beginFrame();
    void endFrame(VkCommandBuffer cmd);
    void submit(uint32_t imageIndex);
    void waitForCurrentFrame();
    void advanceFrame();
    size_t currentFrameIndex() const { return currentFrameIndex_; }

private:
    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
    };

    VulkanContext& context_;
    VulkanCommandManager& commandManager_;
    VulkanSwapchainManager& swapchainManager_;

    std::vector<FrameSync> frames_;
    size_t currentFrameIndex_ = 0;
    VkCommandBuffer currentCommandBuffer_ = VK_NULL_HANDLE;

    void initFrameSync();
    void destroyFrameSync();
    FrameSync& currentFrame();
};
