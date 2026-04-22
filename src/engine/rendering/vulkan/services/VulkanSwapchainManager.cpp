#include "VulkanSwapchainManager.hpp"
#include <algorithm>
#include <stdexcept>
#include <volk.h>
#include "../core/error_handling.hpp"
#include "core/GameWindow.hpp"
#include "rendering/vulkan/core/memory.hpp"

VulkanSwapchainManager::VulkanSwapchainManager(GameWindow& window, const VulkanContext& vulkanContext) {
    createSurface(window, vulkanContext, swapchain_);
    createSwapchain(vulkanContext, swapchain_);
    createImageViews(vulkanContext, swapchain_);
    createDepthResources(vulkanContext, swapchain_);
}

VulkanSwapchain& VulkanSwapchainManager::swapchain() { return swapchain_; }

const VulkanSwapchain& VulkanSwapchainManager::swapchain() const { return swapchain_; }

VulkanSwapchain VulkanSwapchainManager::create(GameWindow& window, const VulkanContext& vulkanContext) const {
    VulkanSwapchain swapchain;
    createSurface(window, vulkanContext, swapchain);
    createSwapchain(vulkanContext, swapchain);
    createImageViews(vulkanContext, swapchain);
    createDepthResources(vulkanContext, swapchain);
    return swapchain;
}

void VulkanSwapchainManager::recreate(const VulkanContext& vulkanContext) {
    vkDeviceWaitIdle(vulkanContext.device);
    cleanupSwapchain(vulkanContext, swapchain_);
    createSwapchain(vulkanContext, swapchain_);
    createImageViews(vulkanContext, swapchain_);
    createDepthResources(vulkanContext, swapchain_);
}

bool VulkanSwapchainManager::acquireNextImage(const VulkanContext& vulkanContext,
                                              VkSemaphore imageAvailableSemaphore,
                                              uint32_t& imageIndex) const {
    VkResult result = vkAcquireNextImageKHR(vulkanContext.device,
                                            swapchain_.swapchain,
                                            UINT64_MAX,
                                            imageAvailableSemaphore,
                                            VK_NULL_HANDLE,
                                            &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    throwIfUnsuccessful(result);
    return true;
}

bool VulkanSwapchainManager::present(VkQueue presentQueue,
                                     VkSemaphore renderFinishedSemaphore,
                                     uint32_t imageIndex) const {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    throwIfUnsuccessful(result);
    return true;
}

void VulkanSwapchainManager::createSurface(GameWindow& window,
                                           const VulkanContext& vulkanContext,
                                           VulkanSwapchain& swapchain) const {
    VkResult surfaceResult = glfwCreateWindowSurface(vulkanContext.instance, window.get(), nullptr, &swapchain.surface);
    throwIfUnsuccessful(surfaceResult);
}

void VulkanSwapchainManager::createSwapchain(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const {
    VkSurfaceCapabilitiesKHR capabilities{};
    throwIfUnsuccessful(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContext.physicalDevice, swapchain.surface, &capabilities));

    swapchain.swapchainExtent = capabilities.currentExtent;
    uint32_t imageCount = std::clamp(3u, capabilities.minImageCount, capabilities.maxImageCount);

    uint32_t formatCount = 0;
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfaceFormatsKHR(
            vulkanContext.physicalDevice, swapchain.surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfaceFormatsKHR(
            vulkanContext.physicalDevice, swapchain.surface, &formatCount, formats.data()));

    VkSurfaceFormatKHR chosenFormat = formats.at(0);
    for (const auto& fmt: formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = fmt;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfacePresentModesKHR(
            vulkanContext.physicalDevice, swapchain.surface, &presentModeCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    throwIfUnsuccessful(vkGetPhysicalDeviceSurfacePresentModesKHR(
            vulkanContext.physicalDevice, swapchain.surface, &presentModeCount, presentModes.data()));

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode: presentModes) {
        if (mode == VK_PRESENT_MODE_FIFO_KHR) {
            chosenPresentMode = mode;
            break;
        }
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = swapchain.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = swapchain.swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    throwIfUnsuccessful(vkCreateSwapchainKHR(vulkanContext.device, &createInfo, nullptr, &swapchain.swapchain));

    throwIfUnsuccessful(vkGetSwapchainImagesKHR(vulkanContext.device, swapchain.swapchain, &imageCount, nullptr));
    swapchain.swapchainImages.resize(imageCount);
    throwIfUnsuccessful(vkGetSwapchainImagesKHR(
            vulkanContext.device, swapchain.swapchain, &imageCount, swapchain.swapchainImages.data()));

    swapchain.swapchainImageFormat = chosenFormat.format;
}

void VulkanSwapchainManager::createImageViews(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const {
    swapchain.swapchainImageViews.resize(swapchain.swapchainImages.size());
    for (size_t i = 0; i < swapchain.swapchainImages.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchain.swapchainImages.at(i);
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain.swapchainImageFormat;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        throwIfUnsuccessful(
                vkCreateImageView(vulkanContext.device, &viewInfo, nullptr, &swapchain.swapchainImageViews.at(i)));
    }
}

void VulkanSwapchainManager::createDepthResources(const VulkanContext& vulkanContext,
                                                  VulkanSwapchain& swapchain) const {
    swapchain.depthBuffer.format = findDepthFormat(vulkanContext);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchain.swapchainExtent.width;
    imageInfo.extent.height = swapchain.swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = swapchain.depthBuffer.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throwIfUnsuccessful(vkCreateImage(vulkanContext.device, &imageInfo, nullptr, &swapchain.depthBuffer.image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanContext.device, swapchain.depthBuffer.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(
            vulkanContext.physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    throwIfUnsuccessful(vkAllocateMemory(vulkanContext.device, &allocInfo, nullptr, &swapchain.depthBuffer.memory));
    vkBindImageMemory(vulkanContext.device, swapchain.depthBuffer.image, swapchain.depthBuffer.memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = swapchain.depthBuffer.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapchain.depthBuffer.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    throwIfUnsuccessful(vkCreateImageView(vulkanContext.device, &viewInfo, nullptr, &swapchain.depthBuffer.view));
}

VkFormat VulkanSwapchainManager::findDepthFormat(const VulkanContext& vulkanContext) const {
    std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vulkanContext.physicalDevice, format, &props);

        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported depth format");
}

void VulkanSwapchainManager::cleanupSwapchain(const VulkanContext& vulkanContext, VulkanSwapchain& swapchain) const {
    for (auto view: swapchain.swapchainImageViews)
        if (view != VK_NULL_HANDLE) vkDestroyImageView(vulkanContext.device, view, nullptr);

    swapchain.swapchainImageViews.clear();

    if (swapchain.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkanContext.device, swapchain.swapchain, nullptr);
        swapchain.swapchain = VK_NULL_HANDLE;
    }

    swapchain.swapchainImages.clear();
}
