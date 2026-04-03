#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <log4cxx/logger.h>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanSwapchainManager {
public:
    VulkanSwapchainManager(const VulkanSwapchainManager&) = delete;
    VulkanSwapchainManager& operator=(const VulkanSwapchainManager&) = delete;
    VulkanSwapchainManager(VulkanSwapchainManager&&) = delete;
    VulkanSwapchainManager& operator=(VulkanSwapchainManager&&) = delete;

    ~VulkanSwapchainManager();
    VulkanSwapchainManager(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

    void recreate();
    bool acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex) const;
    bool present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) const;
    VkExtent2D extent() const;
    VkRenderPass renderPass() const;
    VkSwapchainKHR swapchain() const;
    uint32_t imageCount() const;
    VkImage image(uint32_t index) const;
    VkImageView imageView(uint32_t index) const;
    VkFramebuffer framebuffer(uint32_t index) const;

private:
    void createSurface();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void cleanupSwapchain();

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanSwapchainManager");

    GLFWwindow* window_;
    VkInstance instance_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkSurfaceKHR surface_{VK_NULL_HANDLE};

    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> framebuffers_;

    VkFormat swapchainImageFormat_{VK_FORMAT_UNDEFINED};
    VkExtent2D swapchainExtent_{};
    VkRenderPass renderPass_{VK_NULL_HANDLE};
};
