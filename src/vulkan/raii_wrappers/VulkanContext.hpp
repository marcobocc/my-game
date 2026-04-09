#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext {
public:
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    ~VulkanContext();
    VulkanContext();

    VkInstance getVkInstance() const { return instance_; }
    VkDevice getVkDevice() const { return device_; }
    VkPhysicalDevice getVkPhysicalDevice() const { return physicalDevice_; }
    VkQueue getVkGraphicsQueue() const { return graphicsQueue_; }
    uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }
    VkDescriptorPool getVkDescriptorPool() const { return descriptorPool_; }

private:
    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createDescriptorPool();
    static uint32_t findGraphicsQueueFamily(VkPhysicalDevice device);

    VkInstance instance_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    uint32_t graphicsQueueFamilyIndex_{UINT32_MAX};
    VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
};
