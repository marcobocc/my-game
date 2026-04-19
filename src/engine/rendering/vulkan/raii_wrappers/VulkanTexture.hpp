#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string name;

    VulkanTexture(VkDevice device,
                  VkPhysicalDevice physicalDevice,
                  uint32_t width,
                  uint32_t height,
                  const std::vector<unsigned char>& pixels);
    ~VulkanTexture();
};
