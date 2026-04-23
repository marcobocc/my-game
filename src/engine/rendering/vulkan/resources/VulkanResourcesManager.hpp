#pragma once
#include "VulkanMaterialCache.hpp"
#include "VulkanMeshBuffersCache.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanTextureCache.hpp"
#include "assets/types/material/Material.hpp"
#include "assets/types/shader/Shader.hpp"

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

    VulkanMeshBuffers& getMesh(const MeshData& mesh) const { return meshBuffersCache_.get(mesh); }

    VulkanTexture& getTexture(const Texture& texture) const { return textureCache_.get(texture); }

    VulkanPipeline& getPipeline(const Shader& shader) const { return pipelineCache_.get(shader); }

    VulkanMaterial& getMaterial(const Material& material) { return materialCache_.get(material); }

private:
    VulkanMeshBuffersCache& meshBuffersCache_;
    VulkanTextureCache& textureCache_;
    VulkanPipelineCache& pipelineCache_;
    VulkanMaterialCache& materialCache_;
};
