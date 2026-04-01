#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanPipeline.hpp"

class VulkanPipelinesManager {
public:
    VulkanPipelinesManager(VkDevice device, VkRenderPass renderPass) : device_(device), renderPass_(renderPass) {}

    VulkanPipeline* createOrGetPipeline(const std::string& materialName,
                                        const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
                                        const std::string& vertPath,
                                        const std::string& fragPath) {
        auto it = pipelines_.find(materialName);
        if (it != pipelines_.end()) return &it->second;
        VulkanPipeline pipeline(device_, renderPass_, vertPath, fragPath, vertexInputInfo);
        auto [insertedIt, _] = pipelines_.emplace(materialName, std::move(pipeline));
        return &insertedIt->second;
    }

    VulkanPipeline* getPipeline(const std::string& materialName) {
        auto it = pipelines_.find(materialName);
        if (it != pipelines_.end()) return &it->second;
        return nullptr;
    }

private:
    VkDevice device_;
    VkRenderPass renderPass_;
    std::unordered_map<std::string, VulkanPipeline> pipelines_;
};
