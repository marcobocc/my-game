#pragma once
#include <vector>
#include <vulkan/vulkan.h>

// ------------------------------------------------------------------------
// Device/instance structs
// ------------------------------------------------------------------------

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};

// ------------------------------------------------------------------------
// Buffer structs
// ------------------------------------------------------------------------

struct DepthBuffer {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
};

struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize allocSize = 0;
};

struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
};


// ------------------------------------------------------------------------
// Swapchain structs
// ------------------------------------------------------------------------

struct VulkanSwapchain {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent = {};
    DepthBuffer depthBuffer = {};
};

// ------------------------------------------------------------------------
// Pipeline structs
// ------------------------------------------------------------------------

struct VulkanPipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    uint32_t pushConstantSize = 0;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

// ------------------------------------------------------------------------
// Material structs
// ------------------------------------------------------------------------

struct VulkanMaterial {
    VulkanPipeline* pipeline = nullptr;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

// ------------------------------------------------------------------------
// Per-frame UBO structs
// ------------------------------------------------------------------------

struct VulkanPerFrameUBO {
    VulkanBuffer buffer;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};
