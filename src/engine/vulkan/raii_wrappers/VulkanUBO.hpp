#pragma once

#include <array>
#include <vulkan/vulkan.h>
#include "vulkan/raii_wrappers/VulkanContext.hpp"

class VulkanUBO {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanUBO(const VulkanUBO&) = delete;
    VulkanUBO& operator=(const VulkanUBO&) = delete;
    VulkanUBO(VulkanUBO&&) = delete;
    VulkanUBO& operator=(VulkanUBO&&) = delete;

    ~VulkanUBO();
    VulkanUBO(const VulkanContext& vulkanContext, size_t uboSize);

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_; }
    VkDescriptorSet getDescriptorSet(size_t frameIndex) const { return descriptorSets_.at(frameIndex); }

    void updateUBO(size_t frameIndex, const void* uboData, size_t uboSize) const;

private:
    void createDescriptorSetLayout();
    void createBuffers();
    void createDescriptorSets();

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    size_t uboSize_;

    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> buffers_{};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> bufferMemories_{};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_{};
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
};
