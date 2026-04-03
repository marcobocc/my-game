#pragma once

#include <vulkan/vulkan.h>

class VulkanInstance {
public:
    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;
    VulkanInstance(VulkanInstance&&) = delete;
    VulkanInstance& operator=(VulkanInstance&&) = delete;

    ~VulkanInstance();
    VulkanInstance();
    VkInstance getVkInstance() const;

private:
    VkInstance instance_{VK_NULL_HANDLE};
};
