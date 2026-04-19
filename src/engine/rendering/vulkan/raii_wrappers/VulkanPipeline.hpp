#pragma once
#include <array>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanPipeline {
public:
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    ~VulkanPipeline();
    VulkanPipeline(VulkanPipeline&& other) noexcept;
    VulkanPipeline& operator=(VulkanPipeline&& other) noexcept;
    VulkanPipeline(VkDevice device,
                   const std::vector<char>& vertBytecode,
                   const std::vector<char>& fragBytecode,
                   const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
                   uint32_t pushConstantSize,
                   VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE,
                   VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE,
                   VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                   VkFormat depthFormat = VK_FORMAT_D32_SFLOAT);

    VkPipeline getVkPipeline() const;
    VkPipelineLayout getVkPipelineLayout() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    std::vector<VkShaderModule> shaderModules_;
    uint32_t pushConstantSize_;
    VkFormat colorFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat depthFormat_ = VK_FORMAT_D32_SFLOAT;

    void cleanup();
    void createPipelineLayout(VkDescriptorSetLayout cameraDescriptorSetLayout,
                              VkDescriptorSetLayout textureDescriptorSetLayout,
                              uint32_t pushConstantSize);
    std::array<VkPipelineShaderStageCreateInfo, 2> createShaderStages(const std::vector<char>& vertBytecode,
                                                                      const std::vector<char>& fragBytecode);
    void createGraphicsPipeline(const std::array<VkPipelineShaderStageCreateInfo, 2>& shaderStages,
                                const VkPipelineVertexInputStateCreateInfo& vertexInputInfo);
    void destroyShaderModules();
    VkShaderModule createShaderModule(const std::vector<char>& code) const;
};
