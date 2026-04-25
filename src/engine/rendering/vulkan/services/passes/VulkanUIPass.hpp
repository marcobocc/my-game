#pragma once
#include <vulkan/vulkan.h>
#include "core/ui/UserInterface.hpp"
#include "rendering/vulkan/core/structs.hpp"

class VulkanSwapchainManager;
class GameWindow;

class VulkanUIPass {
public:
    VulkanUIPass(const VulkanContext& vulkanContext,
                 VulkanSwapchainManager& swapchainManager,
                 GameWindow& window,
                 UserInterface& userInterface);
    ~VulkanUIPass();

    void record(VkCommandBuffer cmd, uint32_t imageIndex);

private:
    VulkanSwapchainManager& swapchainManager_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    UserInterface& userInterface_;

    void beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void endRendering(VkCommandBuffer cmd);
};
