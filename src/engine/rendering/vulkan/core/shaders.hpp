#pragma once
#include <span>
#include <utility>
#include <vector>
#include <volk.h>
#include "assets/types/shader/Shader.hpp"
#include "error_handling.hpp"
#include "spirv_reflect.h"
#include "structs.hpp"

inline std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo>
createShader(VkDevice device, const std::vector<char>& bytecode, VkShaderStageFlagBits stage) {
    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = bytecode.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

    VkShaderModule module = VK_NULL_HANDLE;
    throwIfUnsuccessful(vkCreateShaderModule(device, &moduleInfo, nullptr, &module), "Failed to create shader module");

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage;
    stageInfo.module = module;
    stageInfo.pName = "main";

    return {module, stageInfo};
}

inline uint32_t getPushConstantSize(const std::vector<char>& bytecode) {
    SpvReflectShaderModule module{};
    if (bytecode.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error("Invalid SPIR-V size");
    }
    std::vector<uint32_t> spirv(bytecode.size() / sizeof(uint32_t));
    std::memcpy(spirv.data(), bytecode.data(), bytecode.size());
    SpvReflectResult result = spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        return 0;
    }
    uint32_t count = 0;
    result = spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
    uint32_t size = 0;
    if (result == SPV_REFLECT_RESULT_SUCCESS && count > 0) {
        std::vector<SpvReflectBlockVariable*> blocks(count);
        spvReflectEnumeratePushConstantBlocks(&module, &count, blocks.data());
        size = blocks[0]->size; // first block only
    }
    spvReflectDestroyShaderModule(&module);
    return size;
}

inline VulkanPipeline createGraphicsPipeline(VkDevice device,
                                             const Shader& shader,
                                             const VkPipelineVertexInputStateCreateInfo& vertexInput,
                                             const std::vector<VkDescriptorSetLayout>& setLayouts,
                                             VkFormat colorFormat,
                                             VkFormat depthFormat) {
    auto [vertModule, vertStage] = createShader(device, shader.getVertexBytecode(), VK_SHADER_STAGE_VERTEX_BIT);
    auto [fragModule, fragStage] = createShader(device, shader.getFragmentBytecode(), VK_SHADER_STAGE_FRAGMENT_BIT);
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{vertStage, fragStage};

    VulkanPipeline pipeline{};
    pipeline.descriptorSetLayouts = setLayouts;
    pipeline.colorFormat = colorFormat;
    pipeline.depthFormat = depthFormat;
    pipeline.pushConstantSize = getPushConstantSize(shader.getVertexBytecode());

    VkPushConstantRange push{};
    VkPushConstantRange* pushPtr = nullptr;
    if (pipeline.pushConstantSize > 0) {
        push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push.offset = 0;
        push.size = pipeline.pushConstantSize;
        pushPtr = &push;
    }

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = pushPtr ? 1 : 0;
    layoutInfo.pPushConstantRanges = pushPtr;

    throwIfUnsuccessful(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipeline.layout),
                        "Failed to create pipeline layout");

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    // We use a counter-clockwise convention, but we flip Vulkan Y-axis
    // to have a right-handed coordinate system. By flipping Vulkan Y-axis,
    // internally vertices appear as if they are clockwise.
    raster.cullMode = shader.cullDisabled() ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = shader.depthTestDisabled() ? VK_FALSE : VK_TRUE;
    depth.depthWriteEnable = shader.depthWriteDisabled() ? VK_FALSE : VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (shader.alphaBlendEnabled()) {
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;

    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &colorFormat;
    rendering.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = &rendering;
    info.stageCount = static_cast<uint32_t>(shaderStages.size());
    info.pStages = shaderStages.data();
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &inputAssembly;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &msaa;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamicState;
    info.layout = pipeline.layout;

    throwIfUnsuccessful(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline.pipeline),
                        "Failed to create graphics pipeline");

    return pipeline;
}
