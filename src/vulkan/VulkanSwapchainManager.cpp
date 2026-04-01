#include "vulkan/VulkanSwapchainManager.hpp"

#include <algorithm>
#include <stdexcept>
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"

VulkanSwapchainManager::~VulkanSwapchainManager() {
    cleanupSwapchain();
    if (renderPass_ != VK_NULL_HANDLE) vkDestroyRenderPass(device_, renderPass_, nullptr);
    if (surface_ != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance_, surface_, nullptr);
}

VulkanSwapchainManager::VulkanSwapchainManager(GLFWwindow* window,
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
    createRenderPass();
    createFramebuffers();
}

void VulkanSwapchainManager::recreate() {
    vkDeviceWaitIdle(device_);
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createFramebuffers();
}

bool VulkanSwapchainManager::acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex) const {
    VkResult result = vkAcquireNextImageKHR(
            device_, swapchain_, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
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
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    throwIfUnsuccessful(result);
    return true;
}

VkExtent2D VulkanSwapchainManager::extent() const { return swapchainExtent_; }

VkRenderPass VulkanSwapchainManager::renderPass() const { return renderPass_; }

VkSwapchainKHR VulkanSwapchainManager::swapchain() const { return swapchain_; }

uint32_t VulkanSwapchainManager::imageCount() const { return static_cast<uint32_t>(swapchainImages_.size()); }

VkImage VulkanSwapchainManager::image(uint32_t index) const {
    if (index >= swapchainImages_.size()) return VK_NULL_HANDLE;
    return swapchainImages_.at(index);
}

VkImageView VulkanSwapchainManager::imageView(uint32_t index) const {
    if (index >= swapchainImageViews_.size()) return VK_NULL_HANDLE;
    return swapchainImageViews_.at(index);
}

VkFramebuffer VulkanSwapchainManager::framebuffer(uint32_t index) const {
    if (index >= framebuffers_.size()) return VK_NULL_HANDLE;
    return framebuffers_.at(index);
}

void VulkanSwapchainManager::createSurface() {
    VkResult surfaceResult = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
    throwIfUnsuccessful(surfaceResult);
}

void VulkanSwapchainManager::createSwapchain() {
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

void VulkanSwapchainManager::createImageViews() {
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

void VulkanSwapchainManager::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    throwIfUnsuccessful(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_));
}

void VulkanSwapchainManager::createFramebuffers() {
    framebuffers_.resize(swapchainImageViews_.size());
    for (size_t i = 0; i < swapchainImageViews_.size(); ++i) {
        std::array attachments = {swapchainImageViews_.at(i)};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent_.width;
        framebufferInfo.height = swapchainExtent_.height;
        framebufferInfo.layers = 1;

        throwIfUnsuccessful(vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffers_.at(i)));
    }
}

void VulkanSwapchainManager::cleanupSwapchain() {
    for (auto fb: framebuffers_)
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device_, fb, nullptr);

    framebuffers_.clear();

    for (auto view: swapchainImageViews_)
        if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);

    swapchainImageViews_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    swapchainImages_.clear();
}
