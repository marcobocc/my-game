#pragma once
#include <span>
#include "../../../../../modules/asset_management/asset_resources/ShaderResource.hpp"
#include "../../../../../modules/asset_management/asset_resources/TextureResource.hpp"
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

    VulkanMeshBuffers& getMesh(const MeshResource& mesh) const { return meshBuffersCache_.get(mesh); }

    VulkanTexture& getTexture(const TextureResource& texture) const { return textureCache_.get(texture); }

    VulkanPipeline& getPipeline(const ShaderResource& shader,
                                VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                                VkFormat depthFormat = VK_FORMAT_D32_SFLOAT) const {
        return pipelineCache_.get(shader, colorFormat, depthFormat);
    }

    VulkanMaterial& getMaterial(const Material& material) { return materialCache_.get(material); }

    VulkanMaterial&
    getMaterial(const Material& material, std::span<const VkFormat> colorFormats, VkFormat depthFormat) {
        return materialCache_.get(material, colorFormats, depthFormat);
    }

private:
    VulkanMeshBuffersCache& meshBuffersCache_;
    VulkanTextureCache& textureCache_;
    VulkanPipelineCache& pipelineCache_;
    VulkanMaterialCache& materialCache_;
};
