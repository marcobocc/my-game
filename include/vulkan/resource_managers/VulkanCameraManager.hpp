#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "ecs/components/CameraComponent.hpp"

class VulkanCameraManager {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanCameraManager(const VulkanCameraManager&) = delete;
    VulkanCameraManager& operator=(const VulkanCameraManager&) = delete;
    VulkanCameraManager(VulkanCameraManager&&) = delete;
    VulkanCameraManager& operator=(VulkanCameraManager&&) = delete;

    ~VulkanCameraManager();
    VulkanCameraManager(VkDevice device, VkPhysicalDevice physicalDevice);

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_; }
    VkDescriptorSet getDescriptorSet(size_t frameIndex) const { return descriptorSets_[frameIndex]; }

    void updateCameraUBO(size_t frameIndex, const CameraComponent& camera) const;

private:
    struct CameraUBO {
        glm::mat4 view;
        glm::mat4 proj;
    };

    void createDescriptorSetLayout();
    void createBuffers();
    void createDescriptorSets();

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;

    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> buffers_{};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> bufferMemories_{};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_{};
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
};
