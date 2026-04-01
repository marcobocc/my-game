#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanBuffer.hpp"

class VulkanVertexBuffersManager {
public:
    VulkanVertexBuffersManager(VkDevice device, VkPhysicalDevice physicalDevice) :
        device_(device),
        physicalDevice_(physicalDevice) {}

    template<typename T>
    VulkanBuffer* createOrGetVertexBuffer(const std::string& name, const T* data, size_t size) {
        auto it = buffers_.find(name);
        if (it != buffers_.end()) return it->second.get();

        auto buffer = std::make_unique<VulkanBuffer>(device_,
                                                     physicalDevice_,
                                                     size,
                                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                     data);

        auto [insertedIt, _] = buffers_.emplace(name, std::move(buffer));
        return insertedIt->second.get();
    }

    VulkanBuffer* getVertexBuffer(const std::string& name) {
        auto it = buffers_.find(name);
        if (it != buffers_.end()) return it->second.get();
        return nullptr;
    }

private:
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::unordered_map<std::string, std::unique_ptr<VulkanBuffer>> buffers_;
};
