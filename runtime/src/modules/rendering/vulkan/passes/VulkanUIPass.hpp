#pragma once
#include <functional>
#include <vulkan/vulkan.h>
#include "../core/utils/structs.hpp"

class VulkanSwapchainManager;
class GameWindow;

class VulkanUIPass {
public:
    VulkanUIPass(const VulkanContext& vulkanContext, VulkanSwapchainManager& swapchainManager, GameWindow& window);
    ~VulkanUIPass();

    void setDrawCallback(std::function<void()> callback);
    void prepareFrame();
    void record(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent);

private:
    std::function<void()> drawCallback_;

    void beginRendering(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent) const;
    void endRendering(VkCommandBuffer cmd);
};
