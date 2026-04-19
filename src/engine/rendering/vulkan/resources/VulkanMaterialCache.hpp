#pragma once
#include <string>
#include <unordered_map>
#include <volk.h>
#include "VulkanPipelineCache.hpp"
#include "VulkanTextureCache.hpp"
#include "assets/AssetManager.hpp"
#include "core/objects/components/Material.hpp"
#include "rendering/vulkan/core/descriptors.hpp"
#include "rendering/vulkan/core/structs.hpp"

class VulkanMaterialCache {
public:
    VulkanMaterialCache(const VulkanContext& context,
                        VulkanPipelineCache& pipelineCache,
                        VulkanTextureCache& textureCache,
                        AssetManager& assetManager) :
        context_(context),
        pipelineCache_(pipelineCache),
        textureCache_(textureCache),
        assetManager_(assetManager) {}

    VulkanMaterialSet& get(const Material& material) {
        const std::string key = material.shaderName + "|" + material.textureName;
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        const Shader* shader = assetManager_.get<Shader>(material.shaderName);
        VulkanPipeline& pipeline = pipelineCache_.get(*shader);
        VkDescriptorSet descriptorSet = createMaterialDescriptorSet(pipeline, material);
        VulkanMaterialSet materialSet{
                .pipeline = &pipeline,
                .descriptorSet = descriptorSet,
        };
        auto [inserted, _] = cache_.emplace(key, std::move(materialSet));
        return inserted->second;
    }

private:
    VkDescriptorSet createMaterialDescriptorSet(const VulkanPipeline& pipeline, const Material& material) const {
        auto descriptorSetLayout = pipeline.descriptorSetLayouts[1]; // TODO: Get the layout from reflection
        VkDescriptorSet set = createEmptyDescriptorSet(context_.device, context_.descriptorPool, descriptorSetLayout);

        const Texture* texture = assetManager_.get<Texture>(material.textureName);
        const VulkanTexture& tex = textureCache_.get(*texture);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = tex.imageView;
        imageInfo.sampler = tex.sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);

        return set;
    }

    VulkanContext context_;
    VulkanPipelineCache& pipelineCache_;
    VulkanTextureCache& textureCache_;
    AssetManager& assetManager_;
    std::unordered_map<std::string, VulkanMaterialSet> cache_;
};
