#include "vulkan/resource_managers/VulkanVertexBuffersManager.hpp"

VulkanVertexBuffersManager::VulkanVertexBuffersManager(VkDevice device, VkPhysicalDevice physicalDevice) :
    device_(device),
    physicalDevice_(physicalDevice) {}

template<typename T>
VulkanBuffer* VulkanVertexBuffersManager::createOrGetVertexBuffer(const std::string& name, const T* data, size_t size) {
    auto it = buffers_.find(name);
    if (it != buffers_.end()) return it->second.get();

    auto buffer =
            std::make_unique<VulkanBuffer>(device_,
                                           physicalDevice_,
                                           size,
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           data);

    auto [insertedIt, _] = buffers_.emplace(name, std::move(buffer));
    return insertedIt->second.get();
}

VulkanBuffer* VulkanVertexBuffersManager::getVertexBuffer(const std::string& name) {
    auto it = buffers_.find(name);
    if (it != buffers_.end()) return it->second.get();
    return nullptr;
}

// Explicit template instantiation for common types
template VulkanBuffer*
VulkanVertexBuffersManager::createOrGetVertexBuffer<float>(const std::string& name, const float* data, size_t size);
template VulkanBuffer* VulkanVertexBuffersManager::createOrGetVertexBuffer<uint32_t>(const std::string& name,
                                                                                     const uint32_t* data,
                                                                                     size_t size);
