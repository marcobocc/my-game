#pragma once
#include "VulkanMaterialCache.hpp"
#include "VulkanMeshBuffersCache.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanTextureCache.hpp"
#include "assets/types/Material.hpp"
#include "assets/types/Shader.hpp"

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

    VulkanPipeline& getPipeline(const Shader& shader,
                                VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                                VkFormat depthFormat = VK_FORMAT_D32_SFLOAT) const {
        return pipelineCache_.get(shader, colorFormat, depthFormat);
    }

    VulkanMaterial& getMaterial(const Material& material) { return materialCache_.get(material); }

private:
    VulkanMeshBuffersCache& meshBuffersCache_;
    VulkanTextureCache& textureCache_;
    VulkanPipelineCache& pipelineCache_;
    VulkanMaterialCache& materialCache_;
};
