#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "core/ui/UserInterface.hpp"
#include "vulkan/raii_wrappers/VulkanContext.hpp"
#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"

class VulkanImguiRenderer {
public:
    VulkanImguiRenderer(const VulkanContext& vulkanContext,
                        VulkanSwapchain& swapchain,
                        uint32_t imageCount,
                        GLFWwindow* window,
                        UserInterface* userInterface);
    ~VulkanImguiRenderer();

    void recordUIPass(VkCommandBuffer cmd, uint32_t imageIndex);

private:
    VulkanSwapchain& swapchain_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    UserInterface* userInterface_;

    void beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void endRendering(VkCommandBuffer cmd);
};
