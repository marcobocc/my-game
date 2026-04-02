#pragma once
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanErrorHandling.hpp"

class VulkanPipeline {
public:
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    VulkanPipeline(VulkanPipeline&& other) noexcept {
        device_ = other.device_;
        pipeline_ = other.pipeline_;
        layout_ = other.layout_;
        other.pipeline_ = VK_NULL_HANDLE;
        other.layout_ = VK_NULL_HANDLE;
    }

    VulkanPipeline& operator=(VulkanPipeline&& other) noexcept {
        if (this != &other) {
            cleanup();
            device_ = other.device_;
            pipeline_ = other.pipeline_;
            layout_ = other.layout_;
            other.pipeline_ = VK_NULL_HANDLE;
            other.layout_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    ~VulkanPipeline() { cleanup(); }

    VulkanPipeline(VkDevice device,
                   VkRenderPass renderPass,
                   const std::string& vertPath,
                   const std::string& fragPath,
                   const VkPipelineVertexInputStateCreateInfo& vertexInputInfo) :
        device_(device) {
        auto shaderStages = createShaderStages(vertPath.c_str(), fragPath.c_str());
        createPipelineLayout();
        createGraphicsPipeline(renderPass, shaderStages, vertexInputInfo);
        destroyShaderModules();
    }

    VkPipeline getVkPipeline() const { return pipeline_; }
    VkPipelineLayout getVkPipelineLayout() const { return layout_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    std::vector<VkShaderModule> shaderModules_;

    void cleanup() {
        if (pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, pipeline_, nullptr);
        if (layout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, layout_, nullptr);
        destroyShaderModules();
    }

    void createPipelineLayout() {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        auto result = vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &layout_);
        throwIfUnsuccessful(result);
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> createShaderStages(const char* vertPath, const char* fragPath) {
        auto vertShaderCode = readFile(vertPath);
        auto fragShaderCode = readFile(fragPath);
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        shaderModules_.push_back(vertShaderModule);
        shaderModules_.push_back(fragShaderModule);
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertShaderModule;
        vertStage.pName = "main";
        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragShaderModule;
        fragStage.pName = "main";
        return {vertStage, fragStage};
    }

    void createGraphicsPipeline(VkRenderPass renderPass,
                                const std::array<VkPipelineShaderStageCreateInfo, 2>& shaderStages,
                                const VkPipelineVertexInputStateCreateInfo& vertexInputInfo) {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 800.0f;
        viewport.height = 600.0f;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {800, 600};
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr; // Dynamic
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr; // Dynamic

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = layout_;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.pDynamicState = &dynamicState;
        auto result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
        throwIfUnsuccessful(result);
    }

    void destroyShaderModules() {
        for (auto module: shaderModules_)
            vkDestroyShaderModule(device_, module, nullptr);
        shaderModules_.clear();
    }

    static std::vector<char> readFile(const char* filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open file");
        size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();
        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) const {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        throwIfUnsuccessful(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }
};
