#pragma once
#include <map>
#include <span>
#include <string>
#include <tuple>
#include <vector>
#include <volk.h>
#include "../../../../asset_management/asset_types/Shader.hpp"
#include "../utils/error_handling.hpp"
#include "../utils/shaders.hpp"
#include "../utils/structs.hpp"
#include "VulkanVertexLayouts.hpp"

class VulkanPipelineCache {
public:
    explicit VulkanPipelineCache(const VulkanContext& context) : context_(context) {}

    // Single color attachment convenience overload.
    VulkanPipeline& get(const Shader& shader,
                        VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT,
                        bool cullFront = false,
                        bool additiveBlend = false) {
        return get(shader, std::span<const VkFormat>{&colorFormat, 1}, depthFormat, cullFront, additiveBlend);
    }

    VulkanPipeline& get(const Shader& shader,
                        std::span<const VkFormat> colorFormats,
                        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT,
                        bool cullFront = false,
                        bool additiveBlend = false) {
        CacheKey key{shader.name,
                     {colorFormats.begin(), colorFormats.end()},
                     depthFormat,
                     additiveBlend,
                     shader.depthLessOrEqual};
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        // --- Vertex input ---
        VkVertexInputBindingDescription vertexBinding{};
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;
        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (!shader.noVertexInput) {
            if (shader.skinnedVertexLayout) {
                // Skinned layout: pos(3f) + normal(3f) + uv(2f) + boneIdx(4i) + boneWt(4f)
                using SL = VertexLayout_PositionNormalUvSkinned;
                vertexBinding.binding = 0;
                vertexBinding.stride = sizeof(float) * SL::VERTEX_STRIDE;
                vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                auto addAttrib = [&](uint32_t location, uint32_t offset, VkFormat fmt) {
                    VkVertexInputAttributeDescription d{};
                    d.location = location;
                    d.binding = 0;
                    d.format = fmt;
                    d.offset = offset * sizeof(float);
                    vertexAttributes.push_back(d);
                };
                addAttrib(0, SL::POSITION_OFFSET, VK_FORMAT_R32G32B32_SFLOAT);
                addAttrib(1, SL::NORMAL_OFFSET, VK_FORMAT_R32G32B32_SFLOAT);
                addAttrib(2, SL::UV_OFFSET, VK_FORMAT_R32G32_SFLOAT);
                addAttrib(3, SL::BONE_IDX_OFFSET, VK_FORMAT_R32G32B32A32_SINT);
                addAttrib(4, SL::BONE_WT_OFFSET, VK_FORMAT_R32G32B32A32_SFLOAT);
            } else {
                const uint32_t stride = shader.positionColorVertexLayout ? VertexLayout_PositionColor::VERTEX_STRIDE
                                                                         : VertexLayout_PositionNormalUv::VERTEX_STRIDE;
                const auto& attribs =
                        shader.positionColorVertexLayout
                                ? std::span<const VertexAttribute>(VertexLayout_PositionColor::VERTEX_ATTRIBS)
                                : std::span<const VertexAttribute>(VertexLayout_PositionNormalUv::VERTEX_ATTRIBS);
                vertexBinding.stride = sizeof(float) * stride;
                vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                uint32_t location = 0;
                for (const auto& [offset, componentCount]: attribs) {
                    VkFormat format = VK_FORMAT_UNDEFINED;
                    if (componentCount == 2) format = VK_FORMAT_R32G32_SFLOAT;
                    if (componentCount == 3) format = VK_FORMAT_R32G32B32_SFLOAT;
                    if (componentCount == 4) format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    VkVertexInputAttributeDescription desc{};
                    desc.location = location++;
                    desc.binding = 0;
                    desc.format = format;
                    desc.offset = static_cast<uint32_t>(offset * sizeof(float));
                    vertexAttributes.push_back(desc);
                }
            }
            vertexBinding.binding = 0;
            vertexInputState.vertexBindingDescriptionCount = 1;
            vertexInputState.pVertexBindingDescriptions = &vertexBinding;
            vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
            vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();
        }

        // --- Shader modules ---
        auto [vertModule, vertStage] = createShader(context_.device, shader.vertexBytecode, VK_SHADER_STAGE_VERTEX_BIT);
        auto [fragModule, fragStage] =
                createShader(context_.device, shader.fragmentBytecode, VK_SHADER_STAGE_FRAGMENT_BIT);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages{vertStage, fragStage};

        // --- Pipeline object ---
        VulkanPipeline pipeline{};
        pipeline.descriptorSetLayouts =
                reflectDescriptorSetLayouts(context_.device, shader.vertexBytecode, shader.fragmentBytecode);
        pipeline.depthFormat = depthFormat;
        pipeline.pushConstantSize =
                std::max(getPushConstantSize(shader.vertexBytecode), getPushConstantSize(shader.fragmentBytecode));

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
        layoutInfo.setLayoutCount = static_cast<uint32_t>(pipeline.descriptorSetLayouts.size());
        layoutInfo.pSetLayouts = pipeline.descriptorSetLayouts.data();
        layoutInfo.pushConstantRangeCount = pushPtr ? 1 : 0;
        layoutInfo.pPushConstantRanges = pushPtr;
        throwIfUnsuccessful(vkCreatePipelineLayout(context_.device, &layoutInfo, nullptr, &pipeline.layout),
                            "Failed to create pipeline layout");

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology =
                shader.lineTopology ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        std::vector<VkDynamicState> dynamicStateList = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        if (shader.depthBias) dynamicStateList.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateList.size());
        dynamicState.pDynamicStates = dynamicStateList.data();

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = shader.disableCull ? VK_CULL_MODE_NONE
                          : cullFront        ? VK_CULL_MODE_FRONT_BIT
                                             : VK_CULL_MODE_BACK_BIT;
        raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster.lineWidth = 1.0f;
        raster.depthBiasEnable = shader.depthBias ? VK_TRUE : VK_FALSE;

        VkPipelineMultisampleStateCreateInfo msaa{};
        msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth.depthTestEnable = shader.disableDepthTest ? VK_FALSE : VK_TRUE;
        depth.depthWriteEnable = shader.disableDepthWrite ? VK_FALSE : VK_TRUE;
        depth.depthCompareOp = shader.depthLessOrEqual ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (shader.enableAlphaBlend) {
            blendAttachment.blendEnable = VK_TRUE;
            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        } else if (additiveBlend) {
            blendAttachment.blendEnable = VK_TRUE;
            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(colorFormats.size(), blendAttachment);

        VkPipelineColorBlendStateCreateInfo blend{};
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
        blend.pAttachments = blendAttachments.data();

        VkPipelineRenderingCreateInfo rendering{};
        rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        rendering.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        rendering.pColorAttachmentFormats = colorFormats.data();
        rendering.depthAttachmentFormat = depthFormat;

        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.pNext = &rendering;
        info.stageCount = static_cast<uint32_t>(shaderStages.size());
        info.pStages = shaderStages.data();
        info.pVertexInputState = &vertexInputState;
        info.pInputAssemblyState = &inputAssembly;
        info.pViewportState = &viewportState;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &msaa;
        info.pDepthStencilState = &depth;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamicState;
        info.layout = pipeline.layout;
        throwIfUnsuccessful(
                vkCreateGraphicsPipelines(context_.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline.pipeline),
                "Failed to create graphics pipeline");

        vkDestroyShaderModule(context_.device, vertModule, nullptr);
        vkDestroyShaderModule(context_.device, fragModule, nullptr);

        auto [inserted, _] = cache_.emplace(std::move(key), std::move(pipeline));
        return inserted->second;
    }

    void clear() { cache_.clear(); }

private:
    using CacheKey = std::tuple<std::string, std::vector<VkFormat>, VkFormat, bool, bool>;

    const VulkanContext& context_;
    std::map<CacheKey, VulkanPipeline> cache_;
};
