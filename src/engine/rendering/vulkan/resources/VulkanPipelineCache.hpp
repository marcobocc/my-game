#pragma once
#include <string>
#include <unordered_map>
#include "assets/types/mesh/details/VertexLayouts.hpp"
#include "assets/types/shader/Shader.hpp"
#include "rendering/vulkan/core/shaders.hpp"
#include "rendering/vulkan/core/structs.hpp"

// --- Push constants ---
struct PushConstants {
    glm::mat4 modelMatrix;
    glm::vec4 baseColor;
};

class VulkanPipelineCache {
public:
    explicit VulkanPipelineCache(const VulkanContext& context) : context_(context) {}

    VulkanPipeline& get(const Shader& shader) {
        const std::string& name = shader.getName();
        auto it = cache_.find(name);
        if (it != cache_.end()) return it->second;
        return load(shader);
    }

    void clear() { cache_.clear(); }

private:
    VulkanPipeline& load(const Shader& shader) {
        // --- Vertex inputs ---
        VkVertexInputBindingDescription vertexBinding_{};
        vertexBinding_.binding = 0;
        vertexBinding_.stride = sizeof(float) * Vertex_WithLayout_PositionUv::VERTEX_STRIDE;
        vertexBinding_.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> vertexAttributes_;
        uint32_t location = 0;
        for (const auto& [offset, componentCount]: Vertex_WithLayout_PositionUv::VERTEX_ATTRIBS) {
            VkFormat format = VK_FORMAT_UNDEFINED;
            if (componentCount == 2) format = VK_FORMAT_R32G32_SFLOAT;
            if (componentCount == 3) format = VK_FORMAT_R32G32B32_SFLOAT;
            if (componentCount == 4) format = VK_FORMAT_R32G32B32A32_SFLOAT;
            VkVertexInputAttributeDescription desc{};
            desc.location = location++;
            desc.binding = 0;
            desc.format = format;
            desc.offset = static_cast<uint32_t>(offset * sizeof(float));
            vertexAttributes_.push_back(desc);
        }

        VkPipelineVertexInputStateCreateInfo vertexInputState_{};
        vertexInputState_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState_.vertexBindingDescriptionCount = 1;
        vertexInputState_.pVertexBindingDescriptions = &vertexBinding_;
        vertexInputState_.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes_.size());
        vertexInputState_.pVertexAttributeDescriptions = vertexAttributes_.data();

        // --- Frame UBO set layout ---
        VkDescriptorSetLayoutBinding frameUboLayoutBinding{};
        frameUboLayoutBinding.binding = 0;
        frameUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        frameUboLayoutBinding.descriptorCount = 1;
        frameUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo frameUboLayoutInfo{};
        frameUboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        frameUboLayoutInfo.bindingCount = 1;
        frameUboLayoutInfo.pBindings = &frameUboLayoutBinding;

        VkDescriptorSetLayout frameUboLayout = VK_NULL_HANDLE;
        vkCreateDescriptorSetLayout(context_.device, &frameUboLayoutInfo, nullptr, &frameUboLayout);

        // --- Material set layout ---
        VkDescriptorSetLayoutBinding materialLayoutBinding{};
        materialLayoutBinding.binding = 0;
        materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialLayoutBinding.descriptorCount = 1;
        materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
        materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        materialLayoutInfo.bindingCount = 1;
        materialLayoutInfo.pBindings = &materialLayoutBinding;

        VkDescriptorSetLayout materialLayout = VK_NULL_HANDLE;
        vkCreateDescriptorSetLayout(context_.device, &materialLayoutInfo, nullptr, &materialLayout);
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {frameUboLayout, materialLayout};

        // --- Create pipeline ---
        VulkanPipeline pipeline = createGraphicsPipeline(context_.device,
                                                         shader,
                                                         vertexInputState_,
                                                         descriptorSetLayouts,
                                                         VK_FORMAT_B8G8R8A8_UNORM,
                                                         VK_FORMAT_D32_SFLOAT,
                                                         sizeof(PushConstants));

        auto [it, _] = cache_.emplace(shader.getName(), std::move(pipeline));
        return it->second;
    }

    const VulkanContext& context_;
    std::unordered_map<std::string, VulkanPipeline> cache_;
};
