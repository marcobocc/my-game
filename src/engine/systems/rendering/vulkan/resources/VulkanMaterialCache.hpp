#pragma once
#include <string>
#include <unordered_map>
#include <volk.h>
#include "../utils/descriptors.hpp"
#include "../utils/structs.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanTextureCache.hpp"
#include "data/assets/Material.hpp"
#include "systems/assets/AssetManager.hpp"

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

    VulkanMaterial& get(const Material& material) {
        return get(material, std::span<const VkFormat>{}, VK_FORMAT_D32_SFLOAT);
    }

    VulkanMaterial& get(const Material& material, std::span<const VkFormat> colorFormats, VkFormat depthFormat) {
        std::string key = material.getShaderName() + "|" + material.getTextureName();
        for (VkFormat f: colorFormats)
            key += "|" + std::to_string(static_cast<int>(f));
        key += "|" + std::to_string(static_cast<int>(depthFormat));

        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        const Shader* shader = assetManager_.get<Shader>(material.getShaderName());
        VulkanPipeline& pipeline = colorFormats.empty() ? pipelineCache_.get(*shader)
                                                        : pipelineCache_.get(*shader, colorFormats, depthFormat);
        VkDescriptorSet descriptorSet = createTexturesDescriptorSet(pipeline, material);
        auto [inserted, _] = cache_.emplace(key,
                                            VulkanMaterial{
                                                    .pipeline = &pipeline,
                                                    .descriptorSet = descriptorSet,
                                            });
        return inserted->second;
    }

private:
    VkDescriptorSet createTexturesDescriptorSet(const VulkanPipeline& pipeline, const Material& material) const {
        auto descriptorSetLayout = pipeline.descriptorSetLayouts[1]; // TODO: Get the layout from reflection
        VkDescriptorSet set = createEmptyDescriptorSet(context_.device, context_.descriptorPool, descriptorSetLayout);

        const Texture* texture = assetManager_.get<Texture>(material.getTextureName());
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
    std::unordered_map<std::string, VulkanMaterial> cache_;
};
