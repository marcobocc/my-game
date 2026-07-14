#pragma once
#include <array>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <volk.h>
#include "../../../assets/Material.hpp"
#include "../utils/buffers.hpp"
#include "../utils/descriptors.hpp"
#include "../utils/structs.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanTextureCache.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/BuiltinAssetNames.hpp"

class VulkanMaterialCache {
public:
    VulkanMaterialCache(const VulkanContext& context,
                        VulkanPipelineCache& pipelineCache,
                        VulkanTextureCache& textureCache,
                        AssetLoader& assetLoader) :
        context_(context),
        pipelineCache_(pipelineCache),
        textureCache_(textureCache),
        assetLoader_(assetLoader) {}

    VulkanMaterial& get(const Material& material, const std::string& shaderName) {
        return get(material, shaderName, std::span<const VkFormat>{}, VK_FORMAT_D32_SFLOAT);
    }

    VulkanMaterial& get(const Material& material,
                        const std::string& shaderName,
                        std::span<const VkFormat> colorFormats,
                        VkFormat depthFormat) {
        const bool isTerrainBlend = shaderName == GBUFFER_TERRAIN_SHADER;
        std::string key =
                shaderName + "|" +
                (isTerrainBlend ? material.splatMapTexture + terrainLayersKey(material) : material.albedoTexture);
        for (VkFormat f: colorFormats)
            key += "|" + std::to_string(static_cast<int>(f));
        key += "|" + std::to_string(static_cast<int>(depthFormat));

        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        const Shader* shader = assetLoader_.get<Shader>(shaderName);
        VulkanPipeline& pipeline = colorFormats.empty() ? pipelineCache_.get(*shader)
                                                        : pipelineCache_.get(*shader, colorFormats, depthFormat);
        VulkanMaterial vulkanMaterial{.pipeline = &pipeline};
        if (isTerrainBlend) {
            auto [descriptorSet, layerParams] = createTerrainDescriptorSet(pipeline, material);
            vulkanMaterial.descriptorSet = descriptorSet;
            vulkanMaterial.layerParams = layerParams;
        } else {
            vulkanMaterial.descriptorSet = createTexturesDescriptorSet(pipeline, material);
        }
        auto [inserted, _] = cache_.emplace(key, vulkanMaterial);
        return inserted->second;
    }

private:
    std::array<const Material*, 4> resolveLayerMaterials(const Material& material) const {
        const std::array<const std::string*, 4> names{
                &material.layerMaterial0, &material.layerMaterial1, &material.layerMaterial2, &material.layerMaterial3};
        std::array<const Material*, 4> layers{};
        for (size_t i = 0; i < names.size(); ++i) {
            layers[i] = assetLoader_.get<Material>(*names[i]);
            if (!layers[i]) layers[i] = assetLoader_.get<Material>(SOLID_COLOR_MATERIAL);
            if (!layers[i]) throw std::runtime_error("Layer material not found: " + *names[i]);
        }
        return layers;
    }

    // Submaterial values are part of the key so that editing a layer material
    // (tint/tiling/albedo) produces a fresh descriptor set + params UBO.
    std::string terrainLayersKey(const Material& material) const {
        std::string key;
        for (const Material* layer: resolveLayerMaterials(material)) {
            key += "|" + layer->name + "|" + layer->albedoTexture;
            const glm::vec4& t = layer->tint;
            key += "|" + std::to_string(t.r) + "," + std::to_string(t.g) + "," + std::to_string(t.b) + "," +
                   std::to_string(t.a);
            key += "|" + std::to_string(layer->tiling.x) + "," + std::to_string(layer->tiling.y);
            key += "|" + std::to_string(layer->offset.x) + "," + std::to_string(layer->offset.y);
        }
        return key;
    }
    VkDescriptorSet createTexturesDescriptorSet(const VulkanPipeline& pipeline, const Material& material) const {
        auto descriptorSetLayout = pipeline.descriptorSetLayouts[1]; // TODO: Get the layout from reflection
        VkDescriptorSet set = createEmptyDescriptorSet(context_.device, context_.descriptorPool, descriptorSetLayout);

        const Texture* texture = assetLoader_.get<Texture>(material.albedoTexture);
        if (!texture) throw std::runtime_error("Texture not found: " + material.albedoTexture);
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

    // Terrain-blend materials bind the splatmap + each layer submaterial's albedo texture at
    // set=1, bindings 0-4, plus a UBO at binding 5 with per-layer tint/tiling/offset,
    // matching geometry_terrain.frag's declared bindings.
    std::pair<VkDescriptorSet, VulkanBuffer> createTerrainDescriptorSet(const VulkanPipeline& pipeline,
                                                                        const Material& material) const {
        auto descriptorSetLayout = pipeline.descriptorSetLayouts[1];
        VkDescriptorSet set = createEmptyDescriptorSet(context_.device, context_.descriptorPool, descriptorSetLayout);

        const std::array<const Material*, 4> layers = resolveLayerMaterials(material);
        std::array<std::string, 5> textureNames{material.splatMapTexture,
                                                layers[0]->albedoTexture,
                                                layers[1]->albedoTexture,
                                                layers[2]->albedoTexture,
                                                layers[3]->albedoTexture};
        std::array<VkDescriptorImageInfo, 5> imageInfos{};
        std::array<VkWriteDescriptorSet, 6> writes{};
        for (size_t i = 0; i < textureNames.size(); ++i) {
            const Texture* texture = assetLoader_.get<Texture>(textureNames[i]);
            if (!texture) throw std::runtime_error("Texture not found: " + textureNames[i]);
            const VulkanTexture& tex = textureCache_.get(*texture);

            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = tex.imageView;
            imageInfos[i].sampler = tex.sampler;

            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = set;
            writes[i].dstBinding = static_cast<uint32_t>(i);
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].descriptorCount = 1;
            writes[i].pImageInfo = &imageInfos[i];
        }

        // std140 layout of geometry_terrain.frag's LayerParams.
        struct LayerParams {
            glm::vec4 tint[4];
            glm::vec4 tilingOffset[4];
        } params{};
        for (size_t i = 0; i < layers.size(); ++i) {
            params.tint[i] = layers[i]->tint;
            params.tilingOffset[i] =
                    glm::vec4(layers[i]->tiling.x, layers[i]->tiling.y, layers[i]->offset.x, layers[i]->offset.y);
        }
        VulkanBuffer paramsBuffer =
                createBuffer(context_.device,
                             context_.physicalDevice,
                             sizeof(LayerParams),
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &params);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = paramsBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(LayerParams);
        writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[5].dstSet = set;
        writes[5].dstBinding = 5;
        writes[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[5].descriptorCount = 1;
        writes[5].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(context_.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        return {set, paramsBuffer};
    }

    VulkanContext context_;
    VulkanPipelineCache& pipelineCache_;
    VulkanTextureCache& textureCache_;
    AssetLoader& assetLoader_;
    std::unordered_map<std::string, VulkanMaterial> cache_;
};
