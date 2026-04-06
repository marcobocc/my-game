#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <log4cxx/logger.h>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanSwapchain {
public:
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
    VulkanSwapchain(VulkanSwapchain&&) = delete;
    VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

    ~VulkanSwapchain();
    VulkanSwapchain(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

    void recreate();
    bool acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex) const;
    bool present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) const;

    VkExtent2D getVkExtent() const;
    VkRenderPass getVkRenderPass() const;
    VkSwapchainKHR getVkSwapchain() const;
    uint32_t getImageCount() const;
    VkImage getVkImage(uint32_t index) const;
    VkImageView getVkImageView(uint32_t index) const;
    VkFramebuffer getVkFramebuffer(uint32_t index) const;

private:
    void createSurface();
    void createSwapchain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void cleanupSwapchain();
    VkFormat findDepthFormat();

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanSwapchain");

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

    struct DepthBuffer {
        VkImage image{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
        VkFormat format{VK_FORMAT_UNDEFINED};
    } depthBuffer_;
};
