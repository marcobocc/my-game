#include "vulkan/VulkanPipelinesManager.hpp"

VulkanPipelinesManager::VulkanPipelinesManager(VkDevice device, VkRenderPass renderPass) :
    device_(device),
    renderPass_(renderPass) {}

VulkanPipeline* VulkanPipelinesManager::createOrGetPipeline(const std::string& materialName,
                                                            const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
                                                            const std::string& vertPath,
                                                            const std::string& fragPath) {
    auto it = pipelines_.find(materialName);
    if (it != pipelines_.end()) return &it->second;

    VulkanPipeline pipeline(device_, renderPass_, vertPath, fragPath, vertexInputInfo);
    auto [insertedIt, _] = pipelines_.emplace(materialName, std::move(pipeline));
    return &insertedIt->second;
}

VulkanPipeline* VulkanPipelinesManager::getPipeline(const std::string& materialName) {
    auto it = pipelines_.find(materialName);
    if (it != pipelines_.end()) return &it->second;
    return nullptr;
}
