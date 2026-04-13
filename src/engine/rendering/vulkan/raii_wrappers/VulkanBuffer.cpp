#include "rendering/vulkan/raii_wrappers/VulkanBuffer.hpp"
#include <cstring>
#include <volk.h>
#include "rendering/vulkan/VulkanErrorHandling.hpp"

VulkanBuffer::VulkanBuffer(VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           VkDeviceSize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           const void* data) :
    device_(device) {
    createBuffer(physicalDevice, size, usage, properties, data);
}

VulkanBuffer::~VulkanBuffer() {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
}

VkBuffer VulkanBuffer::getVkBuffer() const { return buffer_; }

void VulkanBuffer::createBuffer(VkPhysicalDevice physicalDevice,
                                VkDeviceSize size,
                                VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                const void* data) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throwIfUnsuccessful(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer_));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = getMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    throwIfUnsuccessful(vkAllocateMemory(device_, &allocInfo, nullptr, &memory_));

    if (data) {
        void* mapped = nullptr;
        throwIfUnsuccessful(vkMapMemory(device_, memory_, 0, size, 0, &mapped));
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device_, memory_);
    }

    throwIfUnsuccessful(vkBindBufferMemory(device_, buffer_, memory_, 0));
}

uint32_t
VulkanBuffer::getMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}
