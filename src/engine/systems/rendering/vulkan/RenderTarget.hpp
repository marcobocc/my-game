#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include "utils/structs.hpp"

struct RenderTargetHandle {
    uint32_t index = UINT32_MAX;
    bool isValid() const { return index != UINT32_MAX; }
    bool operator==(const RenderTargetHandle&) const = default;
};

struct RenderTargetData {
    uint32_t width = 0;
    uint32_t height = 0;

    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory = VK_NULL_HANDLE;
    VkImageView colorView = VK_NULL_HANDLE;
    VkSampler colorSampler = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;

    VkImage gbufferAlbedo = VK_NULL_HANDLE;
    VkDeviceMemory gbufferAlbedoMemory = VK_NULL_HANDLE;
    VkImageView gbufferAlbedoView = VK_NULL_HANDLE;
    VkSampler gbufferAlbedoSampler = VK_NULL_HANDLE;

    VkImage gbufferNormal = VK_NULL_HANDLE;
    VkDeviceMemory gbufferNormalMemory = VK_NULL_HANDLE;
    VkImageView gbufferNormalView = VK_NULL_HANDLE;
    VkSampler gbufferNormalSampler = VK_NULL_HANDLE;

    VkImage gbufferDepth = VK_NULL_HANDLE;
    VkDeviceMemory gbufferDepthMemory = VK_NULL_HANDLE;
    VkImageView gbufferDepthView = VK_NULL_HANDLE;

    GeometryRenderContext geometryContext{};
    uint32_t lightingDescSetIndex = UINT32_MAX;

    static constexpr VkFormat COLOR_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
    static constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    static constexpr VkFormat GBUFFER_ALBEDO_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
    static constexpr VkFormat GBUFFER_NORMAL_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

    VkDescriptorSet imguiDescriptorSet = VK_NULL_HANDLE;
};
