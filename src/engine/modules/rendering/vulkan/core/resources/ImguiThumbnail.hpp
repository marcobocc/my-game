#pragma once
#include <vulkan/vulkan.h>

struct ImguiThumbnail {
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;

    explicit operator bool() const { return descriptorSet != VK_NULL_HANDLE; }
};
