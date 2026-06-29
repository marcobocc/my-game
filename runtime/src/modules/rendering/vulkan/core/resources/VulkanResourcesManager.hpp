#pragma once
#include <span>
#include <vulkan/vulkan.h>
#include "../../../../asset_management/asset_types/Shader.hpp"
#include "../../../../asset_management/asset_types/Texture.hpp"
#include "VulkanMaterialCache.hpp"
#include "VulkanMeshBuffersCache.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanTextureCache.hpp"
#include "modules/asset_management/asset_types/Material.hpp"

class VulkanResourcesManager {
public:
    explicit VulkanResourcesManager(VulkanMeshBuffersCache& meshBuffersCache,
                                    VulkanTextureCache& textureCache,
                                    VulkanPipelineCache& pipelineCache,
                                    VulkanMaterialCache& materialCache) :
        meshBuffersCache_(meshBuffersCache),
        textureCache_(textureCache),
        pipelineCache_(pipelineCache),
        materialCache_(materialCache) {}

    VulkanMeshBuffers& getMesh(const Mesh& mesh) const { return meshBuffersCache_.get(mesh); }

    VulkanTexture& getTexture(const Texture& texture) const { return textureCache_.get(texture); }
    VulkanTextureCache& getTextureCache() { return textureCache_; }

    VulkanPipeline& getPipeline(const Shader& shader,
                                VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                                VkFormat depthFormat = VK_FORMAT_D32_SFLOAT,
                                bool cullFront = false,
                                bool additiveBlend = false) const {
        return pipelineCache_.get(shader, colorFormat, depthFormat, cullFront, additiveBlend);
    }

    VulkanPipeline& getPipeline(const Shader& shader,
                                std::span<const VkFormat> colorFormats,
                                VkFormat depthFormat = VK_FORMAT_D32_SFLOAT,
                                bool cullFront = false,
                                bool additiveBlend = false) const {
        return pipelineCache_.get(shader, colorFormats, depthFormat, cullFront, additiveBlend);
    }

    VulkanMaterial& getMaterial(const Material& material, const std::string& shaderName) {
        return materialCache_.get(material, shaderName);
    }

    VulkanMaterial& getMaterial(const Material& material,
                                const std::string& shaderName,
                                std::span<const VkFormat> colorFormats,
                                VkFormat depthFormat) {
        return materialCache_.get(material, shaderName, colorFormats, depthFormat);
    }

private:
    VulkanMeshBuffersCache& meshBuffersCache_;
    VulkanTextureCache& textureCache_;
    VulkanPipelineCache& pipelineCache_;
    VulkanMaterialCache& materialCache_;
};
