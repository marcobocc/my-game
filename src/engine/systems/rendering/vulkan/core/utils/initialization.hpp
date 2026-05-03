#pragma once
#include "structs.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <set>
#include <stdexcept>
#include <vector>
#include <volk.h>
#include "error_handling.hpp"

inline std::vector<const char*> getRequiredExtensions() {
#ifdef __APPLE__
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            "VK_KHR_portability_subset"};
#else
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};
#endif
}

inline bool deviceSupportsExtensions(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions) {
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> available(extCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, available.data());
    std::set<std::string> availableDeviceExtensions;
    for (const auto& [extensionName, _]: available) {
        availableDeviceExtensions.insert(extensionName);
    }
    for (const char* req: requiredExtensions) {
        if (!availableDeviceExtensions.contains(req)) {
            return false;
        }
    }
    return true;
}

inline uint32_t findGraphicsQueueFamily(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }
    return UINT32_MAX;
}

inline VulkanContext initVulkanContext() {
    VulkanContext ctx{};
    throwIfUnsuccessful(volkInitialize());

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GameEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GameEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
    uint32_t layerCount = 1;

    std::vector<const char*> extensions;
    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::copy_n(glfwExts, glfwExtCount, std::back_inserter(extensions));
    extensions.push_back("VK_EXT_debug_utils");
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    throwIfUnsuccessful(vkCreateInstance(&createInfo, nullptr, &ctx.instance));
    volkLoadInstance(ctx.instance);

    // Pick physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());
    std::vector<const char*> requiredExtensions = getRequiredExtensions();
    std::vector<VkPhysicalDevice> suitableDevices;
    for (const auto& device: devices) {
        if (deviceSupportsExtensions(device, requiredExtensions)) {
            suitableDevices.push_back(device);
        }
    }
    if (suitableDevices.empty()) {
        throw std::runtime_error("No suitable physical device found supporting all required extensions");
    }
    ctx.physicalDevice = suitableDevices[0];

    // Create logical device
    ctx.graphicsQueueFamilyIndex = findGraphicsQueueFamily(ctx.physicalDevice);
    if (ctx.graphicsQueueFamilyIndex == UINT32_MAX) {
        throw std::runtime_error("Failed to find graphics queue family");
    }
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = ctx.graphicsQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures features{};
    std::vector<const char*> devExtensions = getRequiredExtensions();

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(devExtensions.size());
    deviceInfo.ppEnabledExtensionNames = devExtensions.data();
    deviceInfo.pNext = &dynamicRenderingFeature;
    throwIfUnsuccessful(vkCreateDevice(ctx.physicalDevice, &deviceInfo, nullptr, &ctx.device));
    volkLoadDevice(ctx.device);
    vkGetDeviceQueue(ctx.device, ctx.graphicsQueueFamilyIndex, 0, &ctx.graphicsQueue);

    // Create descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
                                                   {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 100;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    throwIfUnsuccessful(vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &ctx.descriptorPool));

    return ctx;
}

inline void destroyVulkanContext(VulkanContext& ctx) {
    if (ctx.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx.device);
        if (ctx.descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(ctx.device, ctx.descriptorPool, nullptr);
            ctx.descriptorPool = VK_NULL_HANDLE;
        }
        vkDestroyDevice(ctx.device, nullptr);
        ctx.device = VK_NULL_HANDLE;
    }
    if (ctx.instance != VK_NULL_HANDLE) {
        vkDestroyInstance(ctx.instance, nullptr);
        ctx.instance = VK_NULL_HANDLE;
    }
}
