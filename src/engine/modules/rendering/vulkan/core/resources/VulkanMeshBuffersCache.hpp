#pragma once
#include <unordered_map>
#include "../utils/buffers.hpp"
#include "../utils/structs.hpp"
#include "VulkanVertexLayouts.hpp"
#include "data/assets/Mesh.hpp"

struct VulkanMeshBuffers {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
};

class VulkanMeshBuffersCache {
public:
    explicit VulkanMeshBuffersCache(VulkanContext& context) : context_(context) {}

    VulkanMeshBuffers& get(const Mesh& meshAsset) {
        auto it = cache_.find(meshAsset.getName());
        if (it != cache_.end()) return it->second;

        const std::vector<float> packedVertices = packMeshData(meshAsset);

        constexpr VkMemoryPropertyFlags hostVisible =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VulkanMeshBuffers meshBuffers = {
                .vertexBuffer = createBuffer(context_.device,
                                             context_.physicalDevice,
                                             packedVertices.size() * sizeof(float),
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                             hostVisible,
                                             packedVertices.data()),
                .indexBuffer = createBuffer(context_.device,
                                            context_.physicalDevice,
                                            meshAsset.getIndices().size() * sizeof(uint32_t),
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            hostVisible,
                                            meshAsset.getIndices().data()),
        };

        auto [insertedIt, _] = cache_.emplace(meshAsset.getName(), std::move(meshBuffers));
        return insertedIt->second;
    }

private:
    VulkanContext& context_;
    std::unordered_map<std::string, VulkanMeshBuffers> cache_;
};
