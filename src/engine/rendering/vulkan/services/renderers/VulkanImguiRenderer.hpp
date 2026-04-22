#pragma once
#include <vulkan/vulkan.h>
#include "../../core/structs.hpp"
#include "core/ui/UserInterface.hpp"

class VulkanSwapchainManager;
class GameWindow;

class VulkanImguiRenderer {
public:
    VulkanImguiRenderer(const VulkanContext& vulkanContext,
                        VulkanSwapchainManager& swapchainManager,
                        GameWindow& window,
                        UserInterface& userInterface);
    ~VulkanImguiRenderer();

    void recordUIPass(VkCommandBuffer cmd, uint32_t imageIndex);

private:
    VulkanSwapchainManager& swapchainManager_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    UserInterface& userInterface_;

    void beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void endRendering(VkCommandBuffer cmd);
};
