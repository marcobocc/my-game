#pragma once

#include <log4cxx/logger.h>
#include <vulkan/vulkan.h>

class VulkanDevice {
public:
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    ~VulkanDevice();
    explicit VulkanDevice(VkInstance instance);

    VkDevice getVkDevice() const;
    VkPhysicalDevice getVkPhysicalDevice() const;
    VkQueue getVkGraphicsQueue() const;
    uint32_t getGraphicsQueueFamilyIndex() const;

private:
    bool createLogicalDevice(VkPhysicalDevice physicalDevice);
    void pickPhysicalDevice(VkInstance instance);
    static bool isDeviceSuitable(VkPhysicalDevice device);
    static uint32_t findGraphicsQueueFamily(VkPhysicalDevice device);

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanDevice");

    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};
    uint32_t graphicsQueueFamilyIndex_{UINT32_MAX};
};
