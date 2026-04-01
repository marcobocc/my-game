#pragma once

#include <log4cxx/logger.h>
#include <vulkan/vulkan.h>

class VulkanDebugMessenger {
public:
    VulkanDebugMessenger(const VulkanDebugMessenger&) = delete;
    VulkanDebugMessenger& operator=(const VulkanDebugMessenger&) = delete;
    VulkanDebugMessenger(VulkanDebugMessenger&&) = delete;
    VulkanDebugMessenger& operator=(VulkanDebugMessenger&&) = delete;

    ~VulkanDebugMessenger();
    explicit VulkanDebugMessenger(VkInstance instance);

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                        void* /*userData*/);

    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanDebugMessenger");

    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
};
