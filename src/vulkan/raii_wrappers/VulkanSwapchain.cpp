#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"

#include <algorithm>
#include <stdexcept>
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"

VulkanSwapchain::~VulkanSwapchain() {
    cleanupSwapchain();
    if (depthBuffer_.view != VK_NULL_HANDLE) vkDestroyImageView(device_, depthBuffer_.view, nullptr);
    if (depthBuffer_.image != VK_NULL_HANDLE) vkDestroyImage(device_, depthBuffer_.image, nullptr);
    if (depthBuffer_.memory != VK_NULL_HANDLE) vkFreeMemory(device_, depthBuffer_.memory, nullptr);
    if (surface_ != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance_, surface_, nullptr);
}

VulkanSwapchain::VulkanSwapchain(GLFWwindow* window,
                                 VkInstance instance,
                                 VkPhysicalDevice physicalDevice,
                                 VkDevice device) :
    window_(window),
    instance_(instance),
    physicalDevice_(physicalDevice),
    device_(device) {
    createSurface();
    createSwapchain();
    createImageViews();
    createDepthResources();
}

void VulkanSwapchain::recreate() {
    vkDeviceWaitIdle(device_);
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createDepthResources();
}

bool VulkanSwapchain::acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex) const {
    VkResult result = vkAcquireNextImageKHR(
            device_, swapchain_, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    throwIfUnsuccessful(result);
    return true;
}

bool VulkanSwapchain::present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) const {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    throwIfUnsuccessful(result);
    return true;
}

VkExtent2D VulkanSwapchain::getVkExtent() const { return swapchainExtent_; }

VkSwapchainKHR VulkanSwapchain::getVkSwapchain() const { return swapchain_; }

uint32_t VulkanSwapchain::getImageCount() const { return static_cast<uint32_t>(swapchainImages_.size()); }

VkImage VulkanSwapchain::getVkImage(uint32_t index) const {
    if (index >= swapchainImages_.size()) return VK_NULL_HANDLE;
    return swapchainImages_.at(index);
}

VkImageView VulkanSwapchain::getVkImageView(uint32_t index) const {
    if (index >= swapchainImageViews_.size()) return VK_NULL_HANDLE;
    return swapchainImageViews_.at(index);
}

void VulkanSwapchain::createSurface() {
    VkResult surfaceResult = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
    throwIfUnsuccessful(surfaceResult);
}

void VulkanSwapchain::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities{};
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities));

    swapchainExtent_ = capabilities.currentExtent;
    uint32_t imageCount = std::clamp(3u, capabilities.minImageCount, capabilities.maxImageCount);

    uint32_t formatCount = 0;
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data()));

    VkSurfaceFormatKHR chosenFormat = formats.at(0);
    for (const auto& fmt: formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = fmt;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    throwIfUnsuccessful(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_, surface_, &presentModeCount, presentModes.data()));

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode: presentModes) {
        if (mode == VK_PRESENT_MODE_FIFO_KHR) {
            chosenPresentMode = mode;
            break;
        }
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    throwIfUnsuccessful(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_));

    throwIfUnsuccessful(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr));
    swapchainImages_.resize(imageCount);
    throwIfUnsuccessful(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data()));

    swapchainImageFormat_ = chosenFormat.format;
}

void VulkanSwapchain::createImageViews() {
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages_.at(i);
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainImageFormat_;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        throwIfUnsuccessful(vkCreateImageView(device_, &viewInfo, nullptr, &swapchainImageViews_.at(i)));
    }
}

void VulkanSwapchain::createDepthResources() {
    depthBuffer_.format = findDepthFormat();

    // Create depth image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchainExtent_.width;
    imageInfo.extent.height = swapchainExtent_.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthBuffer_.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throwIfUnsuccessful(vkCreateImage(device_, &imageInfo, nullptr, &depthBuffer_.image));

    // Allocate memory for depth image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device_, depthBuffer_.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanBuffer::getMemoryType(
            physicalDevice_, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    throwIfUnsuccessful(vkAllocateMemory(device_, &allocInfo, nullptr, &depthBuffer_.memory));
    vkBindImageMemory(device_, depthBuffer_.image, depthBuffer_.memory, 0);

    // Create depth image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthBuffer_.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthBuffer_.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    throwIfUnsuccessful(vkCreateImageView(device_, &viewInfo, nullptr, &depthBuffer_.view));
}

VkFormat VulkanSwapchain::findDepthFormat() {
    std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);

        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported depth format");
}

void VulkanSwapchain::cleanupSwapchain() {
    for (auto view: swapchainImageViews_)
        if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);

    swapchainImageViews_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    swapchainImages_.clear();
}
