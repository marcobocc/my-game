#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "../../core/structs.hpp"
#include "core/ui/UserInterface.hpp"

class VulkanSwapchainManager;

class VulkanImguiRenderer {
public:
    VulkanImguiRenderer(const VulkanContext& vulkanContext,
                        VulkanSwapchainManager& swapchainManager,
                        GLFWwindow* window,
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
