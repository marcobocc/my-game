#pragma once
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "../utils/error_handling.hpp"
#include "../utils/structs.hpp"
#include "data/assets/Texture.hpp"

class VulkanTextureCache {
public:
    explicit VulkanTextureCache(VulkanContext& context) : context_(context) {}

    VulkanTexture& get(const Texture& textureAsset) {
        auto it = cache_.find(textureAsset.getName());
        if (it != cache_.end()) return it->second;

        VulkanTexture vulkanTexture = createVulkanTexture(
                static_cast<uint32_t>(textureAsset.getWidth()),
                static_cast<uint32_t>(textureAsset.getHeight()),
                std::vector<unsigned char>(textureAsset.getImageData(),
                                           textureAsset.getImageData() +
                                                   textureAsset.getWidth() * textureAsset.getHeight() * 4));

        auto [insertedIt, _] = cache_.emplace(textureAsset.getName(), std::move(vulkanTexture));
        return insertedIt->second;
    }

private:
    VulkanTexture createVulkanTexture(uint32_t width, uint32_t height, const std::vector<unsigned char>& pixels) {
        VulkanTexture texture{};
        VkDevice device = context_.device;
        VkPhysicalDevice physicalDevice = context_.physicalDevice;
        texture.width = width;
        texture.height = height;

        // --- Create image ---
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        throwIfUnsuccessful(vkCreateImage(device, &imageInfo, nullptr, &texture.image), "Failed to create image");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, texture.image, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        uint32_t memoryTypeIndex = 0;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (memRequirements.memoryTypeBits & 1 << i &&
                memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
                memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
                memoryTypeIndex = i;
                break;
            }
        }
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        throwIfUnsuccessful(vkAllocateMemory(device, &allocInfo, nullptr, &texture.memory),
                            "Failed to allocate image memory");
        vkBindImageMemory(device, texture.image, texture.memory, 0);

        // Upload data
        void* data;
        vkMapMemory(device, texture.memory, 0, allocInfo.allocationSize, 0, &data);
        memcpy(data, pixels.data(), width * height * 4);
        vkUnmapMemory(device, texture.memory);

        // Transition image layout to SHADER_READ_ONLY_OPTIMAL
        // (inline transitionImageLayout logic for simplicity)
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = context_.graphicsQueueFamilyIndex;
        VkCommandPool commandPool;
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool = commandPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);
        vkEndCommandBuffer(commandBuffer);
        VkQueue queue = context_.graphicsQueue;
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(device, commandPool, nullptr);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        throwIfUnsuccessful(vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView),
                            "Failed to create image view");

        // Create sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        throwIfUnsuccessful(vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler),
                            "Failed to create sampler");

        return texture;
    }

    VulkanContext& context_;
    std::unordered_map<std::string, VulkanTexture> cache_;
};
