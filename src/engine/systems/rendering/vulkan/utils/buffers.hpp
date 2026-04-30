#pragma once
#include <algorithm>
#include <volk.h>
#include "error_handling.hpp"
#include "memory.hpp"
#include "structs.hpp"

inline VulkanBuffer createBuffer(VkDevice device,
                                 VkPhysicalDevice physicalDevice,
                                 VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags memoryProperties,
                                 const void* initialData = nullptr) {
    VulkanBuffer buf{};

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    throwIfUnsuccessful(vkCreateBuffer(device, &info, nullptr, &buf.buffer));

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, buf.buffer, &memRequirements);
    buf.allocSize = memRequirements.size;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);
    throwIfUnsuccessful(vkAllocateMemory(device, &allocInfo, nullptr, &buf.memory));
    throwIfUnsuccessful(vkBindBufferMemory(device, buf.buffer, buf.memory, 0));

    if (initialData) {
        void* mapped = nullptr;
        throwIfUnsuccessful(vkMapMemory(device, buf.memory, 0, size, 0, &mapped));
        std::memcpy(mapped, initialData, size);
        vkUnmapMemory(device, buf.memory);
    }

    return buf;
}

inline VulkanBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size) {
    return createBuffer(device,
                        physicalDevice,
                        size,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

inline void updateBuffer(VkDevice device, const VulkanBuffer& buf, const void* data, VkDeviceSize size) {
    void* mapped = nullptr;
    VkDeviceSize safeSize = std::min(size, buf.allocSize);
    vkMapMemory(device, buf.memory, 0, safeSize, 0, &mapped);
    std::memcpy(mapped, data, safeSize);
    vkUnmapMemory(device, buf.memory);
}

inline void destroyBuffer(VkDevice device, VulkanBuffer& buf) {
    if (buf.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, buf.buffer, nullptr);
    if (buf.memory != VK_NULL_HANDLE) vkFreeMemory(device, buf.memory, nullptr);
    buf = {};
}
