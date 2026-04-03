#pragma once
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "vulkan/raii_wrappers/VulkanPipeline.hpp"

class VulkanPipelinesManager {
public:
    VulkanPipelinesManager(const VulkanPipelinesManager&) = delete;
    VulkanPipelinesManager& operator=(const VulkanPipelinesManager&) = delete;
    VulkanPipelinesManager(VulkanPipelinesManager&&) = delete;
    VulkanPipelinesManager& operator=(VulkanPipelinesManager&&) = delete;

    ~VulkanPipelinesManager() = default;
    VulkanPipelinesManager(VkDevice device,
                           VkRenderPass renderPass,
                           VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE);

    VulkanPipeline* createOrGetPipeline(const std::string& materialName,
                                        const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
                                        const std::string& vertPath,
                                        const std::string& fragPath);

    VulkanPipeline* getPipeline(const std::string& materialName);

private:
    VkDevice device_;
    VkRenderPass renderPass_;
    VkDescriptorSetLayout cameraDescriptorSetLayout_;
    std::unordered_map<std::string, VulkanPipeline> pipelines_;
};
