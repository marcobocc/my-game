#include "vulkan/raii_wrappers/VulkanDevice.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"

VulkanDevice::~VulkanDevice() {
    if (device_) {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
    }
}

VulkanDevice::VulkanDevice(VkInstance instance) {
    pickPhysicalDevice(instance);
    createLogicalDevice(physicalDevice_);
}

VkDevice VulkanDevice::getVkDevice() const { return device_; }

VkPhysicalDevice VulkanDevice::getVkPhysicalDevice() const { return physicalDevice_; }

uint32_t VulkanDevice::getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }

VkQueue VulkanDevice::getVkGraphicsQueue() const { return graphicsQueue_; }

bool VulkanDevice::createLogicalDevice(VkPhysicalDevice physicalDevice) {
    graphicsQueueFamilyIndex_ = findGraphicsQueueFamily(physicalDevice);
    if (graphicsQueueFamilyIndex_ == UINT32_MAX) {
        return false;
    }
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = graphicsQueueFamilyIndex_;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures features{};
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME); // Enable negative viewport support
    extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
#ifdef __APPLE__
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, availableExtensions.data());
    auto it = std::ranges::find_if(availableExtensions, [](const VkExtensionProperties& ext) {
        return std::string_view(ext.extensionName, strnlen(ext.extensionName, VK_MAX_EXTENSION_NAME_SIZE)) ==
               "VK_KHR_portability_subset";
    });
    if (it != availableExtensions.end()) {
        extensions.push_back("VK_KHR_portability_subset");
    }
#endif
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceInfo.ppEnabledExtensionNames = extensions.data();
    throwIfUnsuccessful(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device_));
    volkLoadDevice(device_);
    vkGetDeviceQueue(device_, graphicsQueueFamilyIndex_, 0, &graphicsQueue_);
    presentQueue_ = graphicsQueue_;
    return true;
}

void VulkanDevice::pickPhysicalDevice(VkInstance instance) {
    uint32_t deviceCount = 0;
    throwIfUnsuccessful(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
    for (const auto physicalDevice: physicalDevices) {
        if (isDeviceSuitable(physicalDevice)) {
            physicalDevice_ = physicalDevice;
        }
    }
    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    return true;
}

uint32_t VulkanDevice::findGraphicsQueueFamily(VkPhysicalDevice device) {
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
