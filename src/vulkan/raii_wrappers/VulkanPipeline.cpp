#include "vulkan/raii_wrappers/VulkanPipeline.hpp"
#include <stdexcept>
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"

VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept {
    device_ = other.device_;
    pipeline_ = other.pipeline_;
    layout_ = other.layout_;
    shaderModules_ = std::move(other.shaderModules_);
    pushConstantSize_ = other.pushConstantSize_;
    other.pipeline_ = VK_NULL_HANDLE;
    other.layout_ = VK_NULL_HANDLE;
}

VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept {
    if (this != &other) {
        cleanup();
        device_ = other.device_;
        pipeline_ = other.pipeline_;
        layout_ = other.layout_;
        shaderModules_ = std::move(other.shaderModules_);
        pushConstantSize_ = other.pushConstantSize_;
        other.pipeline_ = VK_NULL_HANDLE;
        other.layout_ = VK_NULL_HANDLE;
    }
    return *this;
}

VulkanPipeline::~VulkanPipeline() { cleanup(); }

VulkanPipeline::VulkanPipeline(VkDevice device,
                               VkRenderPass renderPass,
                               const std::vector<char>& vertBytecode,
                               const std::vector<char>& fragBytecode,
                               const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
                               uint32_t pushConstantSize,
                               VkDescriptorSetLayout cameraDescriptorSetLayout) :
    device_(device),
    pushConstantSize_(pushConstantSize) {
    auto shaderStages = createShaderStages(vertBytecode, fragBytecode);
    createPipelineLayout(cameraDescriptorSetLayout, pushConstantSize_);
    createGraphicsPipeline(renderPass, shaderStages, vertexInputInfo);
    destroyShaderModules();
}

VkPipeline VulkanPipeline::getVkPipeline() const { return pipeline_; }

VkPipelineLayout VulkanPipeline::getVkPipelineLayout() const { return layout_; }

void VulkanPipeline::cleanup() {
    if (pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, pipeline_, nullptr);
    if (layout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, layout_, nullptr);
    destroyShaderModules();
}

void VulkanPipeline::createPipelineLayout(VkDescriptorSetLayout cameraDescriptorSetLayout, uint32_t pushConstantSize) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (cameraDescriptorSetLayout != VK_NULL_HANDLE) {
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &cameraDescriptorSetLayout;
    } else {
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
    }

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = pushConstantSize;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    auto result = vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &layout_);
    throwIfUnsuccessful(result);
}

std::array<VkPipelineShaderStageCreateInfo, 2> VulkanPipeline::createShaderStages(const std::vector<char>& vertBytecode,
                                                                                  const std::vector<char>& fragBytecode) {
    VkShaderModule vertShaderModule = createShaderModule(vertBytecode);
    VkShaderModule fragShaderModule = createShaderModule(fragBytecode);
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

void VulkanPipeline::createGraphicsPipeline(VkRenderPass renderPass,
                                            const std::array<VkPipelineShaderStageCreateInfo, 2>& shaderStages,
                                            const VkPipelineVertexInputStateCreateInfo& vertexInputInfo) {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

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
    // Clockwise front faces due to negative viewport with VK_KHR_maintenance1:
    //  - Engine uses right-handed coordinate system with counter-clockwise vertex winding
    //  - Negative viewport height (VK_KHR_maintenance1) flips Y-axis in screen space
    //  - This Y-flip inverts the perceived triangle winding order after viewport transform
    //  - Counter-clockwise vertices now appear clockwise in the flipped coordinate system
    //  - Therefore we configure front faces as clockwise to match the transformed geometry
    // From the user's perspective, they can continue using counter-clockwise winding order
    // in their vertex data and it will work correctly
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = layout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.pDynamicState = &dynamicState;

    auto result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
    throwIfUnsuccessful(result);
}

void VulkanPipeline::destroyShaderModules() {
    for (auto module: shaderModules_)
        vkDestroyShaderModule(device_, module, nullptr);
    shaderModules_.clear();
}

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    throwIfUnsuccessful(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}
