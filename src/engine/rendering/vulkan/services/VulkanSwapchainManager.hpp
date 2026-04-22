#pragma once
#include <log4cxx/logger.h>
#include <vulkan/vulkan.h>
#include "../core/structs.hpp"

class GameWindow;

class VulkanSwapchainManager {
public:
    VulkanSwapchainManager(const VulkanSwapchainManager&) = delete;
    VulkanSwapchainManager& operator=(const VulkanSwapchainManager&) = delete;
    VulkanSwapchainManager(VulkanSwapchainManager&&) = default;
    VulkanSwapchainManager& operator=(VulkanSwapchainManager&&) = default;

    VulkanSwapchainManager(GameWindow& window, const VulkanContext& vulkanContext);

    VulkanSwapchain& swapchain();
    const VulkanSwapchain& swapchain() const;

    void recreate(const VulkanContext& vulkanContext);
    bool acquireNextImage(const VulkanContext& vulkanContext,
                          VkSemaphore imageAvailableSemaphore,
                          uint32_t& imageIndex) const;
    bool present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) const;

private:
    VulkanSwapchain create(GameWindow& window, const VulkanContext& vulkanContext) const;
    void createSurface(GameWindow& window, const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const;
    void createSwapchain(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const;
    void createImageViews(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const;
    void createDepthResources(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const;
    void cleanupSwapchain(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const;
    VkFormat findDepthFormat(const VulkanContext& vulkanContext) const;

    VulkanSwapchain swapchain_;

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanSwapchainManager");
};
