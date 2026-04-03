#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"

class VulkanVertexBuffersManager {
public:
    VulkanVertexBuffersManager(const VulkanVertexBuffersManager&) = delete;
    VulkanVertexBuffersManager& operator=(const VulkanVertexBuffersManager&) = delete;
    VulkanVertexBuffersManager(VulkanVertexBuffersManager&&) = delete;
    VulkanVertexBuffersManager& operator=(VulkanVertexBuffersManager&&) = delete;

    ~VulkanVertexBuffersManager() = default;
    VulkanVertexBuffersManager(VkDevice device, VkPhysicalDevice physicalDevice);

    template<typename T>
    VulkanBuffer* createOrGetVertexBuffer(const std::string& name, const T* data, size_t size);

    VulkanBuffer* getVertexBuffer(const std::string& name);

private:
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::unordered_map<std::string, std::unique_ptr<VulkanBuffer>> buffers_;
};
