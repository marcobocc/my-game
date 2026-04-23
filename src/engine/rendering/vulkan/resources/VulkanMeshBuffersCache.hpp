#pragma once
#include <unordered_map>
#include "assets/types/mesh/Mesh.hpp"
#include "rendering/vulkan/core/buffers.hpp"
#include "rendering/vulkan/core/structs.hpp"

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

        constexpr VkMemoryPropertyFlags hostVisible =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VulkanMeshBuffers meshBuffers = {
                .vertexBuffer = createBuffer(context_.device,
                                             context_.physicalDevice,
                                             meshAsset.getVertices().size() * sizeof(float),
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                             hostVisible,
                                             meshAsset.getVertices().data()),
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
