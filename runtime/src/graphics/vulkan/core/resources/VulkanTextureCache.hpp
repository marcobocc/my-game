#pragma once
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <volk.h>
#include "../../../assets/Texture.hpp"
#include "../utils/buffers.hpp"
#include "../utils/error_handling.hpp"
#include "../utils/structs.hpp"

class VulkanTextureCache {
public:
    explicit VulkanTextureCache(VulkanContext& context) : context_(context) {}

    VulkanTexture& get(const Texture& textureAsset) {
        auto it = cache_.find(textureAsset.name);
        if (it != cache_.end()) return it->second;

        VulkanTexture vulkanTexture = createVulkanTexture(static_cast<uint32_t>(textureAsset.width),
                                                          static_cast<uint32_t>(textureAsset.height));
        uploadAndGenerateMips(vulkanTexture, textureAsset.imageData.data());

        auto [insertedIt, _] = cache_.emplace(textureAsset.name, std::move(vulkanTexture));
        return insertedIt->second;
    }

    // Re-uploads pixel data into the existing cached image for textureAsset.name (no reallocation).
    // Intended for textures whose dimensions never change after creation (e.g. splatmap painting).
    void update(const Texture& textureAsset) {
        auto it = cache_.find(textureAsset.name);
        if (it == cache_.end()) return;
        uploadAndGenerateMips(it->second, textureAsset.imageData.data());
    }

private:
    static uint32_t computeMipLevels(uint32_t width, uint32_t height) {
        return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    }

    VulkanTexture createVulkanTexture(uint32_t width, uint32_t height) const {
        VulkanTexture texture{};
        VkDevice device = context_.device;
        texture.width = width;
        texture.height = height;
        texture.mipLevels = computeMipLevels(width, height);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = texture.mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage =
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        throwIfUnsuccessful(vkCreateImage(device, &imageInfo, nullptr, &texture.image), "Failed to create image");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, texture.image, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = getMemoryType(
                context_.physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        throwIfUnsuccessful(vkAllocateMemory(device, &allocInfo, nullptr, &texture.memory),
                            "Failed to allocate image memory");
        vkBindImageMemory(device, texture.image, texture.memory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = texture.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        throwIfUnsuccessful(vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView),
                            "Failed to create image view");

        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(context_.physicalDevice, &features);
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(context_.physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = features.samplerAnisotropy;
        samplerInfo.maxAnisotropy =
                features.samplerAnisotropy ? std::min(16.0f, properties.limits.maxSamplerAnisotropy) : 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(texture.mipLevels);
        throwIfUnsuccessful(vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler),
                            "Failed to create sampler");

        return texture;
    }

    // Copies pixel data (RGBA8, width*height*4 bytes) into mip 0 through a staging buffer,
    // regenerates the mip chain with blits, and leaves the whole image SHADER_READ_ONLY_OPTIMAL.
    void uploadAndGenerateMips(const VulkanTexture& texture, const unsigned char* pixels) const {
        VkDevice device = context_.device;
        VkDeviceSize size = static_cast<VkDeviceSize>(texture.width) * texture.height * 4;
        VulkanBuffer staging = createBuffer(device,
                                            context_.physicalDevice,
                                            size,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            pixels);

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
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        // All mips -> TRANSFER_DST (image contents may be UNDEFINED on first upload or stale on update).
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = texture.mipLevels;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {texture.width, texture.height, 1};
        vkCmdCopyBufferToImage(cmd, staging.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Blit each mip level from the previous one, transitioning levels as we go.
        barrier.subresourceRange.levelCount = 1;
        int32_t mipWidth = static_cast<int32_t>(texture.width);
        int32_t mipHeight = static_cast<int32_t>(texture.height);
        for (uint32_t i = 1; i < texture.mipLevels; ++i) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            int32_t nextWidth = std::max(mipWidth / 2, 1);
            int32_t nextHeight = std::max(mipHeight / 2, 1);
            VkImageBlit blit{};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            blit.dstOffsets[1] = {nextWidth, nextHeight, 1};
            vkCmdBlitImage(cmd,
                           texture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           texture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            // Previous level is done: TRANSFER_SRC -> SHADER_READ_ONLY.
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            mipWidth = nextWidth;
            mipHeight = nextHeight;
        }

        // Last level: TRANSFER_DST -> SHADER_READ_ONLY.
        barrier.subresourceRange.baseMipLevel = texture.mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        vkEndCommandBuffer(cmd);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(context_.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(context_.graphicsQueue);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        vkDestroyCommandPool(device, commandPool, nullptr);

        destroyBuffer(device, staging);
    }

    VulkanContext& context_;
    std::unordered_map<std::string, VulkanTexture> cache_;
};
