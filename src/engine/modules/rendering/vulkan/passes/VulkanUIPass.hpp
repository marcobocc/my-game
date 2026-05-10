#pragma once
#include <vulkan/vulkan.h>
#include "../core/utils/structs.hpp"
#include "modules/ui/UserInterface.hpp"

class VulkanSwapchainManager;
class GameWindow;

class VulkanUIPass {
public:
    VulkanUIPass(const VulkanContext& vulkanContext,
                 VulkanSwapchainManager& swapchainManager,
                 GameWindow& window,
                 UserInterface& userInterface);
    ~VulkanUIPass();

    void record(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent);

private:
    UserInterface& userInterface_;

    void beginRendering(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent) const;
    void endRendering(VkCommandBuffer cmd);
};
