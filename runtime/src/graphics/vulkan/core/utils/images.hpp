#pragma once
#include <vulkan/vulkan.h>
#include "memory.hpp"
#include "structs.hpp"

inline void allocateRenderTargetImages(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       VkFormat swapchainColorFormat,
                                       VulkanRenderTarget& rt,
                                       VkDeviceMemory& outMemory,
                                       VkSampler& outSampler) {
    auto allocImage = [&](VkFormat format, VkImageUsageFlags usage, VkImage& outImage, VkDeviceMemory& outMem) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {rt.width, rt.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(device, &imageInfo, nullptr, &outImage);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(device, outImage, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &outMem);
        vkBindImageMemory(device, outImage, outMem, 0);
    };

    auto allocView = [&](VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageView& outView) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {aspect, 0, 1, 0, 1};
        vkCreateImageView(device, &viewInfo, nullptr, &outView);
    };

    auto allocSampler = [&](VkFilter filter, VkSampler& outSamplerRef) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = filter;
        samplerInfo.minFilter = filter;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &samplerInfo, nullptr, &outSamplerRef);
    };

    // TRANSFER_SRC allows CPU readback of the render target (e.g. thumbnail previews).
    allocImage(swapchainColorFormat,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               rt.image,
               outMemory);
    allocView(rt.image, swapchainColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, rt.view);
    allocSampler(VK_FILTER_LINEAR, outSampler);
}

inline void freeRenderTargetImages(VkDevice device, VulkanRenderTarget& rt, VkDeviceMemory memory, VkSampler sampler) {
    auto freeImage = [&](VkImage& image, VkDeviceMemory& mem) {
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image, nullptr);
            image = VK_NULL_HANDLE;
        }
        if (mem != VK_NULL_HANDLE) {
            vkFreeMemory(device, mem, nullptr);
            mem = VK_NULL_HANDLE;
        }
    };
    auto freeView = [&](VkImageView& view) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, view, nullptr);
            view = VK_NULL_HANDLE;
        }
    };
    auto freeSampler = [&](VkSampler& samp) {
        if (samp != VK_NULL_HANDLE) {
            vkDestroySampler(device, samp, nullptr);
            samp = VK_NULL_HANDLE;
        }
    };

    freeSampler(sampler);
    freeView(rt.view);
    freeImage(rt.image, memory);
}
